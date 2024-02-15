#include <sys/epoll.h>

#include "Channel.h"
#include "Logger.h"

/*
    POLLIN（描述符可读）
    EPOLLPRI （描述符有紧急事件可读）
    POLLOUT（描述符可写)
    POLLHUP（描述符挂断）
    POLLERR（描述符错误）
*/


const int Channel::kNoneEvent = 0;
const int Channel::KReadEvent = EPOLLIN | EPOLLPRI; //EPOLLIN = 0x001, EPOLLPRI = 0x002,
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}

Channel::~Channel() {}

//
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj; 
    tied_ = true;
}

void Channel::update()
{
    //通过Channel所属的EventLoop，调用poller的相应方法，注册对应事件 
    loop_->updateChannel(this);
    
}

void Channel::remove()
{
    //在Eventloop中删除当前Channel
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
            handleEventWithGuard(receiveTime);
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

//根据poller通知的channel发生的具体事件，由channel调用对应的回调
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    /**
     * 如果事件关断且不可读，说明fd关闭了
     * 判断fd有没有注册关闭回调有则执行
     */
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_) closeCallback_();
    }

    /**
     * 如果事件错误
     * 判断fd有没有注册错误回调有则执行
     */
    if(revents_ & EPOLLERR)
    {
        if(errorCallback_) errorCallback_();
    }

    /**
     * 如果事件可读或为紧急可读事件
     * 判断fd有没有注册读回调有则执行
     */
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_) readCallback_(receiveTime);
    }

    /**
     * 如果事件可写
     * 判断fd有没有注册写回调有则执行 
     */
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_) writeCallback_();
    }
}