#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>


class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const {return started_;}
    pid_t tid() const {return tid_;}
    const std::string& name() const {return name_;}

    static int numCreated() {return numCreated_;}
private:
    void setDefaultName();//设置默认名称

    bool started_;
    bool joined_;//标记线程是否已被等待结束
    std::shared_ptr<std::thread> thread_;//不能直接std::thread thread_;创建线程，这样创建完会自动启动，所以要使用一个指针控制它
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int32_t numCreated_;//对所有线程计数的，所以是静态的
};
