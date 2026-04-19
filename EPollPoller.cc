#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>//close
#include <string.h>//memset

//channel未添加到poller中
const int kNew = -1; //channel 的成员index_ = -1
//channel已添加到poller中
const int kAdded = 1;
//channel从poller中删除
const int kDeleted = 2;

//这里对应epoll_create
EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListsize)  //vector<epoll_event>
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

//映射epoll_wait
//把发生事件的channel通过activechannels告知Eventloop
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    //实际上应该用LOG_DEBUG输出日志更为合理，因为info是什么信息都输出，而poll调用比较多
    //因此会影响性能，并且会输出很多，而debug你要是没有包含那个宏的话，就不会输出
    LOG_INFO("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    //&*events_.begin()的意思是events_.begin()返回一个迭代器，前面加*表示解引用，所以*events_.begin()表示一个值
    //再在前面加&表示取地址，所以&*events_.begin()就是对*events_.begin()这个值取地址
    int numEvents =::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());//获取现在时间


    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n",numEvents);//这里也该用debug
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())//如果event数量等于vector的size了（满了），那就该扩容了
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)//没有监听事件
    {
        LOG_DEBUG("%s timeout! n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)//EINTR外部中断，说明是其他原因引起的
        {
            //重新赋值是因为，errno是全局变量，在处理过程中可能有其他的错误把errno重写了，导致不是这个错误的errno
            errno= saveErrno;


            LOG_ERROR("EPollPoller::poll() err!");
        }
            
    }
    return now;//具体发生事件的时间点
}

//channel update remove => EventLoop updateChannel removeChannel =>Poller updateChannel removeChannel
/**
*           EventLoop => poller.poll
*  ChannelList        Poller
*                   ChannelMap  <fd,channel*>
*channellist里面有全部的channel，但是channelmap里面只有注册过的channel
*所以channellist的channel数量大于等于channelmap
*/

//updatechannel和removechannel映射的是epoll_ctl
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(),channel->events(),index);

    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);//index之前不是knew就是kdeleted，现在改成kadded
        update(EPOLL_CTL_ADD, channel);
    }
    else  //channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())//对任何事不感兴趣
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);//对某个事件感兴趣就要修改（mod）了
        }

    }
}

//从poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded)//添加过的不仅要在channelmap删，还得在epoll删
    {
        update(EPOLL_CTL_DEL, channel);
    }
    

    channel->set_index(kNew);
}
    

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);//因为data.ptr是void类型，所以要强转
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);//EventLoop就拿到了它的poller给它返回的所有发生事件的channe1列表了
    }
}

//operation : add/mod/del
//更新channel通道epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)//删除失败
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else//添加或修改失败
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
        
}