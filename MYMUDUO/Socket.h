#pragma once

#include "noncopyable.h"
#include "InetAddress.h"

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        :sockfd_(sockfd) {}
    ~Socket();

    int fd() const {return sockfd_;}
    //绑定本地地址
    void bindAddress(const InetAddress &localaddr);
    //监听fd
    void listen();
    //从监听队列中接收一个fd
    int accept(InetAddress *peeraddr);

    /**
     * 关闭sockfd的写端
     * 即不能再向sockfd写入任何数据
     * 但是仍然可以从sockfd读取数据
     * 使用这种半关闭方式是为了防止数据漏收
     * 因为对方可能还没有收到所有数据
     * 不能直接close() 
     */
    void shutdownWrite();

    /*on 为1 是打开，为0 是关闭*/
    //设置是否直接发送，不进行tcp缓冲,即禁用 Nagle 算法，减少数据包的延迟
    void setTcpNoDelay(bool on);
    //设置是否允许多个套接字在同一端口绑定
    void setReuseAddr(bool on);
    //设置是否允许多个此案城或线程绑定到统一端口
    void setReusePort(bool on);
    /**
     * 设置是否开启TCP心跳检测
     * 作用：
     * 防止连接因为长时间没有数据交换而断开
     * 它可以在一定时间内自动发送探测包给对方，
     * 如果对方正常回应，则继续保持连接，否则关闭连接。
     */
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};