#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer调用Acceptor获取用户连接，连接成功后可以通过accept函数得到新连接的fd？？？
 * 然后使用TcConnection设置回调，然后设置给Channel，然后注册到Poller上，监听到之后Channel会执行对应的回调操作
 */
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    //返回loop
    EventLoop* getLoop() const {return loop_;}
    //返回name
    const std::string& name() const {return name_;}
    //返回本地addr
    const InetAddress& localAddrress() const {return localAddr_;}
    //返回对端addr
    const InetAddress& peerAddress() const {return peerAddr_;}

    //是否已经连接上，已经连接上返回true
    bool connected() const {return state_ == kConnected;}

    /*用户接口*/
    //发送数据
    void send(const std::string &buf);
    //关闭连接
    void shutdown();

    //设置连接建立和销毁时的回调
    void setConnectionCallback(const ConnectionCallback &cb) {connectionCallback_ = cb;}
    //设置有读写消息时的回调
    void setMessageCallback(const MessageCallback &cb) {messageCallback_ = cb;}
    //设置消息发送(写)完成以后的回调
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {writeCompleteCallback_ = cb;}
    //设置上限回调(例如发送数据缓冲区上限，继续发送就会丢失数据)
    void setHighWaterMarkCallbak(const HighWaterMarkCallbak &cb, size_t highWaterMark) 
    {highWaterMarkCallbak_ = cb; highWaterMark_ = highWaterMark;}
    //设置fd关闭时回调
    void setCloseCallback(const CloseCallback &cb) {closeCallback_ = cb;}

    //建立连接后调用
    void connectEstablished(); //只调用一次
    //断开连接后调用
    void connectDestroyed(); //只调用一次


private:
    /**
     * 已断开连接
     * 正在连接
     * 已连接
     * 正在断开连接
     */
    enum StateE {kDisconnected,
                 kConnecting,
                 kConnected,
                 kDisconnecting};
    void setState(StateE state) {state_ = state;}

    /*内部实现，回调*/
    //处理读操作
    void handleRead(Timestamp receiveTime);
    //处理写操作
    void handleWrite();
    //处理关闭操作
    void handleClose();
    //处理错误操作
    void handleError();

    void sendInLoop(const void *message, size_t len);
    //关闭loop
    void shutdownInLoop();

    //subloop
    EventLoop *loop_;
    const std::string name_;
    //原子变量，记录连接到的状态(正在连接、已连接、断开等)
    std::atomic_int state_;
    //是否正在read
    bool reading_;

    /**
     * accept中也有类似定义
     * accept的Socket、Channel为baseloop
     * 这里为subloop
     */
    //保存用户socket 指向用户socket
    std::unique_ptr<Socket> socket_;
    //保存用户channel 指向用户channel
    std::unique_ptr<Channel> channel_;

    //本地addr
    const InetAddress localAddr_;
    //对端addr
    const InetAddress peerAddr_;

    //连接建立和销毁时的回调
    ConnectionCallback connectionCallback_;
    //有读写消息时的回调
    MessageCallback messageCallback_;
    //消息发送(写)完成以后的回调
    WriteCompleteCallback writeCompleteCallback_;
    //设置上限回调
    HighWaterMarkCallbak highWaterMarkCallbak_;
    //fd关闭回调
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    //接收数据缓冲区
    Buffer inputBuffer_;
    //发送数据缓冲区
    Buffer outputBuffer_; 
};
