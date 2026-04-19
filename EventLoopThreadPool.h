#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool :noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    
    void setThreadNum(int numThreads) {numThreads_ = numThreads;}
    
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    //如果工作在多线程中，baseLoop_默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();
    bool started() const {return started_;}
    const std::string name() const { return name_;}
private:
    EventLoop *baseLoop_;// EventLoop loop;就是用户使用的第一个loop，其实就是那个mainloop
    std::string name_;//线程池的名字
    bool started_;
    int numThreads_;
    int next_;//轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//存储了所有的事件循环线程
    std::vector<EventLoop*>loops_;
};
