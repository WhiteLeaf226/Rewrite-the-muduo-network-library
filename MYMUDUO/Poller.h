#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;

//muduo库中多路事件分发器的核心IO复用模块
class Poller
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    //给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    //添加新Channel
    virtual void updateChannel(Channel *channel) = 0;
    //删除Channel
    virtual void removeChannel(Channel *channel) = 0;

    //判断参数channel是否在当前poller中
    bool hasChannel(Channel *channel) const;

    //EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    //key：sockfd，value：sockfd所属的channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    //定义poller所属的事件循环
    EventLoop *ownerLoop_;
};