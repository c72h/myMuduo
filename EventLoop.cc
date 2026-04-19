#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>//eventfd
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

//防止一个线程创建多个EventLoop，再创建的话就不是空指针了，就不能创建了，类似thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

//定义默认的PollerI0复用接口的超时时间 10秒
const int kPollTimeMs = 10000;

//创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);      
    }
    return evtfd;
}


EventLoop::EventLoop()
    :looping_(false)//还没开始循环呢
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    //, currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)//线程已经存在了
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d n",t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    //设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每一个eventloop都将监听wakeupchannel的EPoLLIN读事件了，内一个subreactor都已一个自己的wakeupchannel
    wakeupChannel_->enableReading();
    //这样mainreactor就能给wakupfd写通知来通知subreactor
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

//开启事件循环
void EventLoop::loop()
{
    looping_=true;
    quit_=false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd一种是client的fd,一种wakeupfd
        pollReturnTime_=poller_->poll(kPollTimeMs, &activeChannels_);

        for (Channel *channel :activeChannels_)
        {
            //Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前EventLoop事件循环需要处理的回调操作
        /**
            IO线程 mainLoop 通过accept 使得fd《=channel 传进subloop
            *mainLoop事先注册一个回调cb（需要subloop来执行） wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作 
            比如说把一个新的channel注册到subloop的poller上这种操作  
        */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

//退出事件循环 1.loop在自己的线程中调用quit 2.在非loop的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_ = true;

    //如果是在其它线程中，调用的quit。比如在一个subloop(woker）中，调用了mainLoop（IO)的quit
    if(!isInLoopThread())
    {
        wakeup();//在非loop的线程中，调用loop的quit，就得先把这个loop唤醒了,因为不知道这个loop啥情况
        //而且，如果不唤醒，那么阻塞状态下，while(!quit)这个循环就不会转圈，也就退出不了循环了
        //所以得唤醒让这个while转圈
    }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())//在当前的loop线程中，执行cb
    {
        cb();
    }
    else//在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}


//把cb放入队列中，唤醒1oop所在的线程，执行cb，因为有可能是好几个其他的loop调用这个loop。所以要把cb放进vector里，一个一个执行
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex>lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }  

    //唤醒相应的，需要执行上面回调操作的1oop的线程了
    if(!isInLoopThread() || callingPendingFunctors_)//callingPendingFunctors_ 为 true 时仍需要唤醒，是为了确保在 doPendingFunctors() 执行期间新添加的任务能够被及时处理，避免延迟。
    //因为你在doPendingFunctors()执行完后又会回到阻塞状态，如果在这个过程中有新的任务被添加，那么就得等再次被唤醒才能执行，这样效率就慢
    {
        wakeup();//唤醒loop所在线程
    }

}



void EventLoop::handleRead()//loop被唤醒之后的重置操作
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);//eventfd read之后就会将数值置零，重置了
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
  }
}

//用来唤醒1oop所在的线程的 向wakeupfd_写一个数据 wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_,&one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }
}


//EventLoop的方法=》Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()//执行回调
{
    //其实本来不写这个functors也可以
    //但是如果不写的话，在执行这个函数的时候，pendingFunctors_会被上锁，执行一个cb删一个，直到执行完
    //这就会导致如果有新的cb想写进pendingFunctors_的话不可能，因为已经被锁住了
    //比如mainloop要往subloop里面注册新的channel，这就是一个cb，那么他就会阻塞，一直注册不进去，系统就会有高延迟
    //所以这里创建一个新的functors，把pendingFunctors_的cb全放进去，这样就可以释放pendingFunctors_去接受新的cb了，两边并发执行，提高效率
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex>lock(mutex_);
        functors.swap(pendingFunctors_);//这样一交换，pendingFunctors_的内容就转移到functors里了，pendingFunctors_就空了
    }

    for (const Functor &functor : functors)
    {
        functor();//执行当前loop需要执行的回调
    }

    callingPendingFunctors_ = false;
}

