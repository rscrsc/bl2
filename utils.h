namespace utils {

std::atomic running(true);
void eventHandler ()
{
    while(running) {
        if (getchar() == '\n') {
            running = false;
        }
    }
}

void dumpTime (std::ostream& os) {
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch) - seconds;
    os << seconds.count() << "." << std::setw(9) << std::setfill('0') << nanoseconds.count();
}

int onPacket(struct ::nfq_q_handle* qh, struct ::nfgenmsg* nfmsg, struct ::nfq_data* nfa, void* data)
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
    //std::cout << "\033[2J\033[H";
    //dumpTime(std::cout);
    //std::cout << "  ";
    //dump(payload, 16, std::cout);
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

};
