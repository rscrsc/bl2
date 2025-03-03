#include <poll.h>
#include <arpa/inet.h> 
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <exception>
#include <thread>
#include <atomic>
#include <chrono>

#include "NFQueue.h"
#include "Udp.h"

constexpr timespec TIMEOUT { .tv_sec=0, .tv_nsec=1'000'000 };

std::atomic running(true);

void dumpTime (std::ostream& os) {
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch) - seconds;
    os << seconds.count() << "." << std::setw(9) << std::setfill('0') << nanoseconds.count();
}

void dump(void* addr, size_t size, std::ostream& os) {
    unsigned char* bytePtr = static_cast<unsigned char*>(addr);
    size_t bytesPerLine = 16;

    for (size_t i = 0; i < size; i += bytesPerLine) {
        os << std::setw(8) << std::setfill('0') << std::hex << i << ": ";

        for (size_t j = 0; j < bytesPerLine; ++j) {
            if (i + j < size) {
                os << std::setw(2) << std::setfill('0') << (int)bytePtr[i + j] << " ";
            } else {
                os << "   ";
            }
        }

        os << " |";
        for (size_t j = 0; j < bytesPerLine; ++j) {
            if (i + j < size) {
                char c = bytePtr[i + j];
                os << (std::isprint(c) ? c : '.');
            }
        }
        os << "|" << std::endl;
    }
    os << std::resetiosflags(std::ios::adjustfield | std::ios::basefield | std::ios::floatfield | std::ios::skipws) << std::setfill(' ') << std::setw(0) << std::setprecision(6) << std::fixed;
}

void eventHandler ()
{
    while(running) {
        if (getchar() == '\n') {
            running = false;
        }
    }
}

int onPacket(struct nfq_q_handle* qh, struct nfgenmsg* nfmsg, struct nfq_data* nfa, void* data)
{
    nfqnl_msg_packet_hdr* pkt = nfq_get_msg_packet_hdr(nfa);
    if (pkt == nullptr) {
        throw std::runtime_error("unable to access packet");
    }
    unsigned char* payload = nullptr;
    int pktLen = nfq_get_payload(nfa, &payload);
    if (pktLen == 0 || payload == nullptr) {
        throw std::runtime_error("unable to get packet payload");
    }
    if (nfq_set_verdict(qh, ntohl(pkt->packet_id), NF_ACCEPT, 0u, nullptr) == -1) {
        throw std::runtime_error("unable to set verdict");
    }
    std::cout << "\033[2J\033[H";
    dumpTime(std::cout);
    std::cout << "  ";
    dump(payload, 16, std::cout);
    return 0;
}

class UniqueFd {
public:
    int fd;
    UniqueFd (int fd)
    : fd(fd) {};
    ~UniqueFd ()
    {
        close(fd);
    }
};

int main() {
Udp udp("192.168.69.139", 12345);
udp.send();
return 0;
std::thread eh(eventHandler);
try{
    NFQueue nfq(0, &onPacket);
    UniqueFd epollUFd(epoll_create1(NOFLAG));
    int epollFd = epollUFd.fd;
    if (epollFd == -1) {
        throw std::runtime_error("unable to create epoll fd");
    }
    {
        epoll_event ev{ .events = EPOLLIN, .data = {.fd=nfq.sockFd} };
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
            throw std::runtime_error("unable to add an event to epoll");
        }
    }
    UniqueFd timerUFd(timerfd_create(CLOCK_MONOTONIC, 0));
    int timerFd = timerUFd.fd;
    if (timerFd == -1) {
        throw std::runtime_error("unable to create timer fd");
    }
    itimerspec timerSpec{
        .it_interval = {.tv_sec=0, .tv_nsec=0},
        .it_value = TIMEOUT
    };
    timerfd_settime(timerFd, NOFLAG, &timerSpec, nullptr);
    {
        epoll_event ev{ .events = EPOLLIN, .data = {.fd=timerFd} };
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
            throw std::runtime_error("unable to add an event to epoll");
        }
    }
    while (running) {
        std::array<epoll_event, 2> events;
        int rPoll = epoll_wait(epollFd, events.data(), events.size(), -1); 
        if (rPoll < 0) {
            throw std::runtime_error("failed waiting epoll");
        }
        for (int i = 0; i < rPoll; ++i) {
            if (events[i].data.fd == timerFd) {
                uint64_t expirations;
                read(timerFd, &expirations, sizeof(expirations));
                timerfd_settime(timerFd, NOFLAG, &timerSpec, nullptr);
                std::cout << "\033[2J\033[H";
                dumpTime(std::cout);
                std::cout << "  ";
                std::cout << "========= No Packet Sent =========" << std::endl;
            } else if (events[i].data.fd == nfq.sockFd) {
                nfq.process();
            }
        }
    }
    eh.join();
} catch (std::exception &e) {
    std::cerr << "[Error] " << e.what() << std::endl;
    eh.join();
    return 1;
}
    return 0;
}
