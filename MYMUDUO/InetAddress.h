#pragma once

#include <arpa/inet.h>
#include <string>

class InetAddress
{
private:
    sockaddr_in addr_;
public:
    explicit InetAddress(in_port_t port = 10010, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr);

    std::string toIp() const;
    std::string toIpPort() const;
    in_port_t toPort() const;

    const sockaddr_in* getSockAddr() const;

    void setSockAddr(const sockaddr_in &addr) {addr_ = addr;}
};
