#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>//为了回调
#include <memory>

class EventLoop;

//理清eventloop，channel，poller之间的关系 《=reactor模型对应demultiplex（多路事件分发器

//channel理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN,EPOLLOUT事件
//还绑定了poller返回的具体事件（最终发生的事件
//这些事件最终要向poller中注册
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;//事件回调
    using ReadEventCallback = std::function<void(Timestamp)>;//只读事件回调

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的。调用相应的回调方法
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    //发生后的事件由poller向channel通知，channel得到fd的事件以后调用相应的回调 
    void setReadCallback(ReadEventCallback cb) {readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {errorCallback_ = std::move(cb);}

    //防止当channel被手动remove掉，channel还在执行回调操作(就是上面那4个)
    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}//channel不能监听fd发生的事件，是poller在监听
    //所以这个是poller向channel返回fd事件的一个接口

    //设置fd响应的事件状态
    void enableReading() { events_ |= kReadEvent; update();}//update调用epoll_ctl把fd感兴趣事件添加到epoll里面
    //既然添加事件用 |=，那么取消事件通常用 &= ~
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting() { events_ |= kWriteEvent; update();}
    void disableWriting() { events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ = kNoneEvent; update();}

    //返回fd当前事件状态
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    bool isWriting() const {return events_ & kWriteEvent;}//在当前的 events_ 状态中，有没有包含 kWriteEvent 这个事件
    bool isReading() const {return events_ & kReadEvent;}

    int index() {return index_;}
    void set_index(int idx) {index_ = idx;}

    //one loop per thread
    //设置和cpu核数一样多的eventloop线程
    //1个线程有1个eventloop，1个eventloop有一个poller，1个poller可以监听很多channel
    EventLoop* ownerLoop() {return loop_;}
    void remove();
private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime);//处理受保护的事件

    //每个事件只占用二进制中的一位。
    static const int kNoneEvent;//不对任何事件感兴趣
    static const int kReadEvent;//对读事件感兴趣
    static const int kWriteEvent;//对写事件感兴趣

    EventLoop *loop_;//事件循环
    const int fd_;//fd ,poller监听的对象
    int events_;//注册fd感兴趣的事件，fd对某个事件感兴趣要通过epoll_ctl添加、修改、删除
    int revents_;//poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;


    //因为channel通道里面能够获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};