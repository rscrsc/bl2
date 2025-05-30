#include "Faker.h"

Faker::Faker (const std::string ip, const uint16_t port)
: ip (ip),
  port (port)
{
    sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd < 0) {
        throw std::runtime_error("Faker: failed creating socket");
    }
    peerAddrInfo.sin_family = AF_INET;
    peerAddrInfo.sin_port = htons(port);
    peerAddrInfo.sin_addr.s_addr = inet_addr(ip.c_str());
}

void Faker::send ()
{
    sendto(sockFd, payload.data(), payload.size(), 0, (sockaddr*)&peerAddrInfo, sizeof(peerAddrInfo));
}

Faker::~Faker ()
{
    close(sockFd);
}
