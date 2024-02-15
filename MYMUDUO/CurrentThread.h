#pragma once

#include <unistd.h>
#include <sys/syscall.h>

//在多个文件中向同一个命名空间添加成员
namespace CurrentThread
{
    /**
     * 在.cpp文件中对t_cachedTid进行了初始化
     * .h文件要是用需要加extern 
     * thread_local表示此变量是线程局部存储，每个线程都有独立的副本
     */
    extern thread_local int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0, 0))
            cacheTid();
        return t_cachedTid;
    }
}