#ifndef NFQUEUE_H
#define NFQUEUE_H

#define NOFLAG 0

#include <unistd.h>
#include <sys/epoll.h>

#include <cstdint>
#include <memory>
#include <array>
#include <stdexcept>
#include <fstream>

#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

class NFQueue {
private:
    nfq_callback* callback;
    uint64_t getWaitLength ();
public:
    std::unique_ptr<nfq_handle, decltype(&nfq_close)> ctx{nullptr, &nfq_close};
    std::unique_ptr<nfq_q_handle, decltype(&nfq_destroy_queue)> queue{nullptr, &nfq_destroy_queue}; 
    uint16_t queueNum;
    int sockFd;
    epoll_event ev{};
    alignas(8) std::array<char, 4096> packetBuffer;

    NFQueue (uint16_t queueNum, nfq_callback* cb);
    void process ();
    bool isEmpty ();
};

#endif // NFQUEUE_H
