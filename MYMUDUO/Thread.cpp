#include <semaphore.h>

#include "Thread.h"
#include "CurrentThread.h"

std::atomic_int32_t Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    :started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func)),
    name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach(); //线程分离
    }
}


void Thread::start() //一个Thread对象，记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    //开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        //获取线程tid
        tid_ = CurrentThread::tid();
        sem_post(&sem); 
        func_(); //开启一个新线程，专门执行线程函数
    })); 

    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join(); //等待子线程结束
}


void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread%d", num);
    }
}