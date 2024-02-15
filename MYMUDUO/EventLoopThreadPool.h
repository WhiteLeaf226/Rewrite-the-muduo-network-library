#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"


class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    //设置线程数量
    void setThreadNum(int numThreads) {numThreads_ = numThreads;}

    //启动numThreads_个事件线程
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    //获取下一个loop，通过轮询的方式
    EventLoop* getNextLoop();

    //获取所有的loop
    std::vector<EventLoop*> getAllLoops();
    bool started() const {return started_;}
    const std::string name() const {return name_;}

private:
    //主loop
    EventLoop *baseLoop_;
    //线程池名字
    std::string name_;
    //是否启动线程池中的线程
    bool started_;
    //设置线程池中线程数
    int numThreads_;
    //下一个线程
    int next_;
    //包含了所有创建事件的线程
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    /**
     * 线程中EventLoop的指针，startLoop可以的到EventLoop*
     * 存放线程池中各个线程所存放的事件的地址
     */
    std::vector<EventLoop*> loops_;
};