#include <functional>
#include <error.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "TcpConnection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include "Logger.h"

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("TcpConnection loop is null! \n");
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    :loop_(CheckLoopNotNull(loop)),
    name_(nameArg),
    state_(kConnecting),
    reading_(true),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024) //64M
{
    //下面给channel设置相应的回调函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n ", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf)
{   
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    //接收write函数返回值，判断数据发送情况
    ssize_t nwrote = 0;
    //需要发送的数据长度
    size_t remaining = len;
    //是否出现错误
    bool faultError = false;

    
    if(state_ == kDisconnected) //已经断开连接
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    /**
     * 对写事件不感兴趣，即这是第一次发送
     * 并且发送缓冲区没有待发送的数据
     * 那就直接发送 
     */
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        
        if(nwrote >= 0) //发送成功
        {
            remaining = len - nwrote;
            //这里数据全部发送完成，就不用再向channel注册EPOLLOUT事件
            if(remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else //出错
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK) //不是因为非阻塞正常的返回导致nwrote < 0的话
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET) //如果通道关闭或连接重置，表明发生了错误
                {
                    faultError = true;
                }
            }
        }
    }

    /**
     * 没有出错，但是数据没有发送完
     * 剩余数据需要保存到缓冲区中
     * 然后给channel注册EPOLLOUT事件
     * poller发现tcp发送缓冲区有剩余空间
     * 会通知对应channel执行writeCallback_回调
     * 也就是调用TcpConnection::handleWrite()方法
     * 把发送缓冲区中的数据全部发送完成
     */
    if(!faultError && remaining > 0) 
    {
        //目前发送缓冲区，剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallbak_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallbak_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting(); //一定要注册channel写事件，否则poller不会给channel通知EPOLLOUT
        }
    }
}

void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}   

void TcpConnection::shutdownInLoop()
{
    /**
     * 如果对读事件不感兴趣，就执行 
     * 因为可能在执行shutdownInLoop时还在handleWrite
     */
    if(!channel_->isWriting())
    {
        socket_->shutdownWrite(); //关闭写端
    }
}

void TcpConnection::connectEstablished()
{
    //设置为已建立连接状态
    setState(kConnected);
    channel_->tie(shared_from_this());
    //向poller注册channel的epollin事件
    channel_->enableReading(); 

    //新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected)
    {
        //设置成已断开连接状态
        setState(kDisconnected);
        //把channel的所有感兴趣事件，从poller中del中
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    //把channel重poller中删除掉
    channel_->remove();
}

//处理读操作
void TcpConnection::handleRead(Timestamp receiveTime)
{
    //保存错误信息
    int savedError = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedError);
    if(n > 0)
    {
        //已连接用户有读事件产生，调用设置的对应回调函数
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0) 
    {
        //用户断开
        handleClose();
    }
    else //出现错误
    {
        errno = savedError;
        LOG_ERROR("TcpConnection::handlead");
        handleError();
    }
}
//处理写操作
void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    { 
        //保存错误信息
        int savedError = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedError);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                //如果设置了数据发送完后的回调
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                //如果正在断开连接
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handlerWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection::handlerWrite fd=%d is down, no more writing \n", channel_->fd());
    }
}

/**
 * poller通知channe的calback方法
 * 最终回调到Tcpconnection::handleClose()
 */
//处理关闭操作
void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); //执行连接关闭的回调
    /**
     * 执行关闭连接的回调 
     * 此回调是在TcpServer::newConnection设置的
     * 设置的是TcpServer::removeConnection
     */
    closeCallback_(connPtr); 
}
//处理错误操作
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    //<0 发生错误，否则未发生错误
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}
