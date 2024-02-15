#include <errno.h>
#include <unistd.h>
#include <strings.h>

#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

/**
 * Channel未添加到Poller中
 * Channel的成员index_ = -1，创建Channel未添加到Poller中
 */
const int kNew = -1; 
//Channel已添加到Poller中
const int kAdded = 1;
//Channel从Poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    :Poller(loop), 
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)), //创建epoll实例并设置为CLOEXEC模式，以提高安全性
    events_(kInitEventListSize)
{
    if(epollfd_ < 0)
        LOG_FATAL("epoll_create error:%d \n", errno);
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}


Timestamp EPollPoller::poll(int timestampMs, ChannelList *activeChannels) 
{
    //应该用LOG_DEBUG更合理
    LOG_INFO("func%s => fd total count:%lu",__FUNCTION__ ,channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &(*events_.begin()), 
                    static_cast<int>(events_.size()), timestampMs);
    //执行多个epoll_wait都可能出错进而访问errno所以需要记录errno
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    //有事件发生,numEvents请求IO的文件描述符数目
    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);

        //发生事件个数等于列表长度则需要扩容，因为可能大于
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0) //超时
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else //发生了错误
    {
        if(saveErrno != EINTR)
        {
            /**
             * errno为全局的errno有可能被其他的调用更改
             * 在MUDUO源码中LOG_SYSERR调用的是全局errno
             * 所以要输出当前errno需要用当前的赋值给全局 
             */
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() errno");
        }
    }
    return now;
}   

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    /**
     * 为什么是前numEvents是就绪的
     * epoll_wait返回就绪fd数目
     * 并且epoll_wait会把就绪的事件存放在events_
     */
    for(int i = 0; i < numEvents; ++i)
    {
        Channel *channle = static_cast<Channel *>(events_[i].data.ptr);
        channle->set_revents(events_[i].events);
        activeChannels->push_back(channle);  //EventLoop就拿到了它的Poller返回的所有发生事件的Channel列表
    }
}

/*
    EventLoop 管理了Channel和Poller
    Poller管理了 ChannelMap和epollfd
    一个EventLoop有多个Channel
    一个EventLoop有一个Poller
*/

void EPollPoller::updateChannel(Channel *channel) 
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd=%d events=%d index=%d \n",__FUNCTION__, channel->fd(), channel->events(), index);
    
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else //Channel已经在Poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }

    
}

void EPollPoller::removeChannel(Channel *channel) 
{
    int fd = channel->fd();
    channels_.erase(fd);
    
    LOG_INFO("func=%s => fd=%d \n",__FUNCTION__, channel->fd());

    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}



void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof(event));
    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = channel->fd();
    event.data.ptr = channel;
    

    /**
     *  epoll_ctl添加、修改或删除fd的事件设置
     *  传入epoll实例，执行的操作，操作的fd，event包含了事件类型和数据
     */
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}