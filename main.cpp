#include <arpa/inet.h> 
#include <sys/timerfd.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#include "NFQueue.h"
#include "Faker.h"

#include "args.hxx"

#include "utils.h"

int main(int argc, char** argv) {
std::thread eh(utils::eventHandler);

args::ArgumentParser parser("Default prompt");
args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
args::ValueFlag<double> timeIntv(parser, "timeIntv", "Time interval (in sec)", {'t'});
args::PositionalList<std::string> ips(parser, "ips", "Client IP list");
try{
    parser.ParseCLI(argc, argv);
    std::cout << "Time: " << args::get(timeIntv) << std::endl;
    timespec TIMEOUT { .tv_sec=0, .tv_nsec=1'000'000 };
    for (auto& ip : args::get(ips)) {
        std::cout << ip << std::endl;
    }
    NFQueue nfqA(0, &utils::onPacket);
    NFQueue nfqB(1, &utils::onPacket);
    utils::UniqueFd epollUFd(epoll_create1(NOFLAG));
    int epollFd = epollUFd.fd;
    if (epollFd == -1) {
        throw std::runtime_error("unable to create epoll fd");
    }
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, nfqA.ev.data.fd, &nfqA.ev) == -1) {
        throw std::runtime_error("unable to add an event to epoll");
    }
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, nfqB.ev.data.fd, &nfqB.ev) == -1) {
        throw std::runtime_error("unable to add an event to epoll");
    }
    utils::UniqueFd timerUFd(timerfd_create(CLOCK_MONOTONIC, 0));
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
    Faker udpA("192.168.69.139", 12345);
    Faker udpB("192.168.69.67", 12345);
    int queueNow = 1;
    while (utils::running) {
        std::array<epoll_event, 3> evBuffer;
        int rPoll = epoll_wait(epollFd, evBuffer.data(), evBuffer.size(), -1); 
        if (rPoll < 0) {
            throw std::runtime_error("failed waiting epoll");
        }
        bool hasA = false, hasB = false;
        for (int i = 0; i < rPoll; ++i) {
            if (evBuffer[i].data.fd == nfqA.sockFd) {
                nfqA.process();
                hasA = true;
            }
            if (evBuffer[i].data.fd == nfqB.sockFd) {
                nfqB.process();
                hasB = true;
            }
            if (evBuffer[i].data.fd == timerFd) {
                queueNow = (queueNow + 1) % 2;

                uint64_t expirations;
                read(timerFd, &expirations, sizeof(expirations));
                timerfd_settime(timerFd, NOFLAG, &timerSpec, nullptr);
                if (queueNow == 0 && (!hasA)) {
                    udpA.send();
                    //std::cout << "\033[2J\033[H";
                    //dumpTime(std::cout);
                    //std::cout << "  ";
                    //std::cout << "Sent complement UDP to A" << std::endl;
                } else if (queueNow == 1 && (!hasB)) {
                    udpB.send();
                    //std::cout << "\033[2J\033[H";
                    //dumpTime(std::cout);
                    //std::cout << "  ";
                    //std::cout << "Sent complement UDP to B" << std::endl;
                }
            }
        }
    }
    eh.join();
} catch (args::ParseError &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    eh.join();
    return 1;
} catch (args::ValidationError &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    eh.join();
    return 1;
} catch (std::exception &e) {
    std::cerr << "[Error] " << e.what() << std::endl;
    eh.join();
    return 1;
}
    return 0;
}
