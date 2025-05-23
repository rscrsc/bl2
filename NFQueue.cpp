#include "NFQueue.h"
#include <string>
#include <cerrno>
#include <stdexcept>

uint64_t NFQueue::getWaitLength ()
{
    // TODO: The length when checked does not have to be the length we really need
    // TODO: Maybe too much overhead in parsing stats with istream
    std::ifstream statFile("/proc/net/netfilter/nfnetlink_queue");
    if (!statFile.is_open()) {
        throw std::runtime_error("failed to read queue stats");
    }
    std::string line;
    uint16_t uselessLinesCnt = this->queueNum;
    while (uselessLinesCnt--) {
        std::getline(statFile, line);
    }
    uint64_t res{};
    for (int i = 0; i < 3; ++i) {
        statFile >> res;
    }
    return res;
}

bool NFQueue::isEmpty ()
{
    if (this->getWaitLength() == 0LL) {
        return true;
    } else {
        return false;
    }
}

void NFQueue::process ()
{
    ssize_t rvSize = recv(sockFd, packetBuffer.data(), packetBuffer.size(), NOFLAG);
    if (rvSize >= 0) {
        nfq_handle_packet(ctx.get(), packetBuffer.data(), rvSize);
    } else {
        if (errno == ENOBUFS) return;
        throw std::runtime_error(std::string("failed at recv: ")+std::to_string(errno));
    }
}

NFQueue::NFQueue (uint16_t queueNum, nfq_callback* cb)
: queueNum (queueNum),
  callback (cb)
{
    ctx.reset(nfq_open());
    if (ctx.get() == nullptr) {
        throw std::runtime_error("NFQueue: unable to open nfqueue");
    }
    if (nfq_unbind_pf(ctx.get(), AF_INET) < 0) {
        throw std::runtime_error("NFQueue: unable to unbind existing nfqueue from protocol AF_INET");
    }
    if (nfq_bind_pf(ctx.get(), AF_INET) < 0) {
        throw std::runtime_error("NFQueue: unable to bind nfqueue to protocol AF_INET");
    }
    queue.reset(nfq_create_queue(ctx.get(), queueNum, this->callback, this));
    if (queue.get() == nullptr) {
        throw std::runtime_error("NFQueue: unable to create queue");
    }
    if (nfq_set_mode(queue.get(), NFQNL_COPY_PACKET, 0xffff) < 0) {
        throw std::runtime_error("NFQueue: unable to set queue mode");
    }
    sockFd = nfq_fd(ctx.get());
    ev.events = EPOLLIN;
    ev.data.fd = sockFd;
}
