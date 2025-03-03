#ifndef UDP_H
#define UDP_H

#include <unistd.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <cstdint>
#include <string>
#include <array>
#include <stdexcept>

class Udp
{
private:
    int sockFd = -1;
    sockaddr_in peerAddrInfo;
    const std::array<uint8_t,4> payload = { 0x18, 0x18, 0x42, 0x69 };
    std::string ip;
    uint16_t port;
public:
    Udp (const std::string ip, const uint16_t port);
    ~Udp ();

    void send();
};

#endif // UDP_H
