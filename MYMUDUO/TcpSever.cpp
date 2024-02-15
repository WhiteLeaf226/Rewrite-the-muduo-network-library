#include <strings.h>

#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("mainLoop is null! \n");
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    :loop_(CheckLoopNotNull(loop)),
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(),
    messageCallback_(),
    nextConnId_(1)
{
    //当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto &item : connections_)
    {
        //这个局部的强智能指针对象，离开循环，就自动释放了new出来的TcpConnection对象资源
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        //销毁连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if(started_++ == 0) //防止一个TcpServer对象被start多次
    {
        threadPool_->start(threadInitCallback_); //启动底层的loop线程池
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

/**
 * 当有新连接时会调用此回调
 * 首先会组装这个连接的名字name + ip + port + id
 * 然后创建这条连接（new TcpConnection）
 * 然后给这条连接绑定了相应的回调
 */
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
              name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    //通过sockfd获取连接到本机的对端ip地址和端口号
    sockaddr_in local;
    ::bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }

    InetAddress localAddr(local);

    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;

    /**
     * 下面的回调都是用户设置给TcpServer的
     * TcpServer又设置给TcpConnection
     * TcpConnection又设置给channel
     * channel注册到poller上
     * 最后poller会通知channel执行对应回调
     */
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    //设置了关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    
    /**
     * 直接调用connectEstablished
     * 只要就新连接就会创建TcpConnection对象
     * 然后调用connectEstablished
     */
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}


void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n",
              name_.c_str(), conn->name().c_str());

    //从map表中删除TcpConnection
    connections_.erase(conn->name());

    //获取连接所在的loop
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
