#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    :loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
{

}


EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if(loop_!= nullptr)
    {
        loop_->quit();
        thread_.join();
    }

}
EventLoop* EventLoopThread::startLoop()
{
    thread_.start();//启动底层的新线程
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex>lock(mutex_);
        while(loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop=loop_;
    }
    return loop;
}

//下面这个方法，实在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{
    EventLoop loop;//创建一个独立的eventloop，和上面的线程是——对应的，oneloopperthread
    
    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_= &loop;
        cond_.notify_one();//通知一个线程
    }
    
    loop.loop();// EventLoop loop 开启了=>Poller.poll

    //poller返回了
    std::unique_lock<std::mutex> lock(mutex_);
    //按照逻辑来说可以不加锁，但是你也不能保证其他线程不会访问到这个loop_临界变量，加锁还是安全一点
    loop_ = nullptr;
}