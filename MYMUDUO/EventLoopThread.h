#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

#include "noncopyable.h"
#include "Thread.h"

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
        const std::string &name = std::string());
    ~EventLoopThread();

    //启动循环
    EventLoop* startLoop();
private:
    //线程函数
    void threadFunc();

    EventLoop *loop_;
    //是否退出循环
    bool exiting_;
    Thread thread_;
    //互斥锁
    std::mutex mutex_;
    //条件变量
    std::condition_variable cond_;
    //回调
    ThreadInitCallback callback_;

};