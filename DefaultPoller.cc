#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>//获取环境变量getenv


//属于一个公共源文件，添加poll或者epoll实现的源文件就没有关系了
//但是poller.h是一个基类层的，最好不要引用派生类（这个是一个很好的思想
Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL"))//getenv就是给一个键，返回对应的值
    {
        return nullptr;//生成poll的实例
    }
    else{
        return new EPollPoller(loop);//生成epoll的实例
    }

}