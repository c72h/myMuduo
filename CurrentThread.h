#pragma once

#include <unistd.h>
#include <sys/syscall.h>
//获取当前线程tid
namespace CurrentThread
{
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0,0))
        {
            cacheTid();
        }
        //如果有（不等于0）那就直接返回
        return t_cachedTid;
    }

}