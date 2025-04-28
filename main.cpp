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
    int timeIntvSec = static_cast<int>(args::get(timeIntv));
    int timeIntvNsec = static_cast<int>((args::get(timeIntv) - timeIntvSec)*1'000'000'000);
    timespec TIMEOUT { .tv_sec=timeIntvSec, .tv_nsec=timeIntvNsec };
    std::vector<std::string>& ipStrs = args::get(ips);

    std::vector<NFQueue> nfqs;
    nfqs.reserve(ipStrs.size());
    for (int i = 0; i < ipStrs.size(); ++i) {
        nfqs.emplace_back(i, &utils::onPacket);
    }

    utils::UniqueFd epollUFd(epoll_create1(NOFLAG));
    int epollFd = epollUFd.fd;
    if (epollFd == -1) {
        throw std::runtime_error("unable to create epoll fd");
    }

    for (int i = 0; i < ipStrs.size(); ++i) {
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, nfqs[i].ev.data.fd, &(nfqs[i].ev)) == -1) {
            throw std::runtime_error("unable to add an event to epoll");
        }
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

    std::vector<Faker> fakers{};
    fakers.reserve(ipStrs.size());
    for (auto& ipStr : ipStrs) {
        fakers.emplace_back(ipStr, 12345);
    }

    int queueNow = nfqs.size() - 1;
    while (utils::running) {

        std::vector<epoll_event> evBuffer;
        evBuffer.resize(ipStrs.size() + 1, {});
        int rPoll = epoll_wait(epollFd, evBuffer.data(), evBuffer.size(), -1); 
        if (rPoll < 0) {
            throw std::runtime_error("failed waiting epoll");
        }

        std::vector<bool> isRecv;
        isRecv.resize(nfqs.size(), {});
        for (int i = 0; i < rPoll; ++i) {
            for (int j = 0; j < nfqs.size(); ++j) {
                if (evBuffer[i].data.fd == nfqs[j].sockFd) {
                    nfqs[j].process();
                    isRecv[j] = true;
                }
            }
            if (evBuffer[i].data.fd == timerFd) {
                queueNow = (queueNow + 1) % nfqs.size();

                uint64_t expirations;
                read(timerFd, &expirations, sizeof(expirations));
                timerfd_settime(timerFd, NOFLAG, &timerSpec, nullptr);
                if (!isRecv[queueNow]) {
                    fakers[queueNow].send();
                    //std::cout << "\033[2J\033[H";
                    //dumpTime(std::cout);
                    //std::cout << "  ";
                    //std::cout << "Sent complement UDP to A" << std::endl;
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
