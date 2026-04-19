#pragma once

//就是reactor反应堆的角色

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"//tid

class Channel;
class Poller;

//事件循环类，主要包含两个大模块 channel poller（epoll的抽象）
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;//写一个回调

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();
    Timestamp pollReturnTime() const {return pollReturnTime_;}

    //在当前loop中执行回调cb
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程执行cb（因为当前loop可能不在创建他的线程里面）
    void queueInLoop(Functor cb);

    //用来唤醒loop所在的线程
    void wakeup();

    //Eventloop的方法 调用 poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    //判断eventloop是否在自己的线程里面
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid(); }//证明eventloop就在创建他的loop线程里面

private:

    void handleRead();//loop被唤醒之后使用的，用来置零
    void doPendingFunctors();//执行回调

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; //原子操作，通过cAs实现的
    std::atomic_bool quit_;//标识退出loop循环

    const pid_t threadId_;//记录当前loop所在线程的id  one loop per thread
    
    Timestamp pollReturnTime_;//poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;//唤醒，就是mainreactor怎么唤醒subreactor呢，因为client传一个channel，subreactor可能在睡觉，需要唤醒它
    //主要作用，当mainreactor获取一个新用户的channel，通过轮询算法选择一个subreactor，通过该成员唤醒subreactor处理channel

    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    //Channel *currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否有需要执行的回调操作
    std::vector<Functor>pendingFunctors_;//存储loop需要执行的所有的回调操作
    std::mutex mutex_;//互斥锁，用来保护上面vector容器的线程安全操作

};