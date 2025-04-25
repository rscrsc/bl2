#ifndef FAKER_H
#define FAKER_H

#include <unistd.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <cstdint>
#include <string>
#include <array>
#include <stdexcept>

class Faker
{
private:
    int sockFd = -1;
    sockaddr_in peerAddrInfo;
    const std::array<uint8_t,4> payload = { 0x18, 0x18, 0x42, 0x69 };
    std::string ip;
    uint16_t port;
public:
    Faker (const std::string ip, const uint16_t port);
    ~Faker ();

    void send();
};

#endif // FAKER_H
