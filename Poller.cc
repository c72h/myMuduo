#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel * channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

//那么为什么没有newDefaultPoller的实现呢
//因为这个要生成一个具体的IO复用对象，比如是poll，比如是epoll
//那么既然是一个具体的，那就必须包含这两个的头文件才能返回一个具体的
//但是你这个newDefaultPoller是在基类里的，怎么能引用派生类的头文件呢
//这样设计不好。所以要再分一个.cc文件出去，这个就是DefaultPoller.cc文件