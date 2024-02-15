#include <string.h>

#include "InetAddress.h"

InetAddress::InetAddress(in_port_t port, std::string ip)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    addr_.sin_port = htons(port);
}

InetAddress::InetAddress(const sockaddr_in &addr)
{
    addr_ = addr;
}

std::string InetAddress::toIp() const
{
    char buf[64];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

//以"ip:port"的形式返回
std::string InetAddress::toIpPort() const
{
    char buf[64];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    in_port_t port = ntohs(addr_.sin_port);
    sprintf(buf + strlen(buf), ":%u", port);
    return buf;
}

in_port_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

const sockaddr_in* InetAddress::getSockAddr() const
{
    return &addr_;
}