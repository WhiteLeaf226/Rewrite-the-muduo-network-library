#pragma once

#include <vector>
#include <sys/epoll.h>


#include "Poller.h"
#include "Timestamp.h"

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    //override指明某函数是虚函数重写
    ~EPollPoller() override;

    //重写基类Poller的抽象方法
    //EventLoop调用poller.poll
    Timestamp poll(int timestampMs, ChannelList *activeChannels) override;
    //更新Channel(根据index选择对应的update)
    void updateChannel(Channel *channel) override;
    //从Poller中删除Channel
    void removeChannel(Channel *channel) override;
private:
    //EventList初始的长度
    static const int kInitEventListSize = 16;

    //填写活跃的链接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    //更新channel通道
    void update(int operation, Channel *channel);


    using EventList = std::vector<epoll_event>;

    //epoll实例的文件描述符
    int epollfd_;
    //epoll内存放的事件
    EventList events_;

};
