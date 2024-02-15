#pragma once


#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    //设置新用户连接时执行的回调
    void setNewConnectionCallback(const NewConnectionCallback &cb) {newConnectionCallback_ = std::move(cb);}

    bool listenning() const {return listenning_;}
    //启动监听
    void listen();

private:
    //接收到可读事件时，执行的回调
    void handleRead();

    //Acceptor用的是用户定义的baseLoop，也称作mainLoop
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    //新用户连接成功后执行的回调
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};