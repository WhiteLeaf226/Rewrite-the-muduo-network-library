#include <stdlib.h>

#include "Poller.h"
#include "EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    /**
     * getenv获取环境变量的值
     * 环境变量：MUDUO_USE_POLL
     * 存在使用poll实现poller否则使用epoll
     */
    if(getenv("MUDUO_USE_POLL"))
    {
        return nullptr; //生成poll实例
    }
    else
    {
        return new EPollPoller(loop); //生成epoll实例
    }
}