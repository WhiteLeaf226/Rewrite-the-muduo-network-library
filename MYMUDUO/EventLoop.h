#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <unistd.h>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    //启动事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    //在当前loop执行cb
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    //唤醒loop所在的线程
    void wakeup();

    //eventloop的方法调用poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    //判断eventloop对象是否在自己的线程中
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}
private:
    void handleRead();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;   

    /**
     * 原子类型bool值
     * 原子类型是多线程同步机制
     * 能保证操作共享变量不被其它线程干扰
     * 是否启动循环
     */
    std::atomic_bool looping_;
    //标识退出loop循环
    std::atomic_bool quit_;
    //记录当前loop所在的线程id
    const pid_t threadId_;
    //poller返回发生事件的channels的时间点
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;
    
    //当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    //标识当前loop是否有需要执行的回调操作
    std::atomic_bool callingPendingFunctors_;
    //存储loop需要执行的所有的回调操作
    std::vector<Functor> pendingFunctors_;
    //保护vector<Functor>线程安全  
    std::mutex mutex_;
};
