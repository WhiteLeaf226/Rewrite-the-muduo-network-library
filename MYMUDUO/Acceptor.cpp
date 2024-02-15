#include <arpa/inet.h>

#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

/**
 * loop用来构造acceptChannel_，loop是baseloop
 * Acceptor保存了新用户的socket和channel对象
 * 构造函数会设置读回调
 * 那么当调用Acceptor::listen时就可以监听新用户的连接，它会启用之前设置的读回调
 * 之后调用Acceptor::handleRead就可以得到一个新用户的连接
 * 如果设置setNewConnectionCallback，那么在得到新用户连接时，
 * 也就是Acceptor::handleRead中回去执行设置的newConnectionCallback_
 */

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sockfd < 0)
    {
        LOG_FATAL("listen socket create err:%d \n", errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    :loop_(loop),
    acceptSocket_(createNonblocking()), //socket
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); //bind
    //TcpServer::start() Acceptor.listen
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;

    acceptSocket_.listen(); //lister
    acceptChannel_.enableReading(); //acceptChannel_=>poller
}

void Acceptor::handleRead()
{
    //peerAddr是accept的回调参数
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(newConnectionCallback_)
        {
            //轮询找到subLoop(子循环)，唤醒，分发当前的新客户端的Channel
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("accept err:%d \n", errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("sockfd reached limit! \n");
        }
    }

}