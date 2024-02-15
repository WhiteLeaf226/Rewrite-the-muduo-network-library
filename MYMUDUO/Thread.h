#pragma once

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

#include "noncopyable.h"

class Thread : noncopyable
{  
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    //启动线程
    void start();
    //执行join
    void join();

    //获取started_(是否启动线程)
    bool started() const {return started_;}
    //获取tid_(线程编号)
    pid_t tid() const {return tid_;}
    //获取name_(线程名字)
    const std::string& name() const {return name_;}

    //获取numCreated_(线程数量)
    static int numCreated() {return numCreated_;}
private:
    //设置默认名称
    void setDefaultName();

    //是否启动
    bool started_;
    //是否join
    bool joined_;
    //保存字线程对象
    std::shared_ptr<std::thread> thread_;
    //线程编号
    pid_t tid_;
    //回调
    ThreadFunc func_;
    //线程名字
    std::string name_;
    //记录线程数量
    static std::atomic_int32_t numCreated_;
};

