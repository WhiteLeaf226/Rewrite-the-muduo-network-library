#pragma once

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    /**
     * 枚举类型
     * 第一个值为0，后续值为前一个+1
     * 枚举在编译阶段将名字替换成对应的值
     * 宏在预处理阶段将名字替换成对应的值
     */
    enum Option
    {
        //不重用端口
        kNoReusePort,
        //重用端口
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);

    ~TcpServer();

    //设置线程初始化的回调函数（一般默认即不设置）
    void setThreadInitCallback(const ThreadInitCallback &cb) {threadInitCallback_ = cb;}
    //设置有新连接时的回到函数
    void setConnectionCallback(const ConnectionCallback &cb) {connectionCallback_ = cb;}
    //设置有读写消息时的回调函数
    void setMessageCallback(const MessageCallback &cb) {messageCallback_ = cb;}
    //设置消息发送完成后的回到函数
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {writeCompleteCallback_ = cb;}

    
    //设置底层subloop的个数
    void setThreadNum(int numThreads);

    //开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    //删除连接，调用removeConnectionInLoop
    void removeConnection(const TcpConnectionPtr &conn);
    //删除连接执行的操作
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    //baseLoop 用户定义的loop
    EventLoop *loop_; 

    //ip:port
    const std::string ipPort_;
    const std::string name_;

    //运行在mainLoop，用来监听连接事件
    std::unique_ptr<Acceptor> acceptor_;

    //one loop per thread
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    //有新连接时的回调
    ConnectionCallback connectionCallback_;
    //有读写消息时的回调
    MessageCallback messageCallback_;
    //消息发送(写)完成以后的回调
    WriteCompleteCallback writeCompleteCallback_;

    //loop线程初始化的回调
    ThreadInitCallback threadInitCallback_;

    std::atomic_int started_;

    int nextConnId_;
    //保存所有的连接
    ConnectionMap connections_;
};
