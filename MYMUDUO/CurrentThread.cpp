#include "CurrentThread.h"

namespace CurrentThread
{
    thread_local int t_cachedTid = 0;

    void cacheTid()
    {
        if(cacheTid == 0)
        {
            //static_cast<pid_t>()类型转换操作符，将()转换为pid_t类型
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}