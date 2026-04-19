#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;  // 不对任何事件感兴趣
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;  // 对读事件感兴趣
const int Channel::kWriteEvent = EPOLLOUT; // 对写事件感兴趣

//eventloop : channellist poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{  
}

//channel的tie方法什么时候调用过? 一个TcpConnection新连接创建的时候TcpConnection=>Channel
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

//当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
//通过channel所属的eventloop调用poller的相应方法，注册fd的event事件（因为channel里面没有poller的类
//所以只能通过eventloop调用
void Channel::update()
{

    loop_->updateChannel(this);//this把channel传进去了
}

//在channel所属的eventloop中，当前的channel删除掉
void Channel::remove()
{

    loop_->removeChannel(this);
}


void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else{
        handleEventWithGuard(receiveTime);
    }
}

//根据poller通知的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))//异常
    {
        if(closeCallback_)
        {
            closeCallback_();//关了
        }
    }

    if(revents_ & EPOLLERR)//错误
    {
        if(errorCallback_){
            errorCallback_();//这里EventCallback的function是void()
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI))//可读
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);//这里ReadEventCallback的function是void(receiveTime)
        }
    }

    if(revents_ & EPOLLOUT)//可写
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}