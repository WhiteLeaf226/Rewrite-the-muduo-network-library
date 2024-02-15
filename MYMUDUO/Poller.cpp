#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop)
    :ownerLoop_(loop) {}

bool Poller::hasChannel(Channel *channel) const
{
    //查找channel的fd返回指向该fd的迭代器
    auto it = channels_.find(channel->fd());
    //不为end则找到了fd，再判断存储的channel是不是传递的channel
    return it != channels_.end() && it->second == channel;
}