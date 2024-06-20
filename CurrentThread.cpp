//
// Created by 27262 on 2024/6/19.
//

#include "CurrentThread.h"
#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread {
    //意味着每个线程都有自己的 t_cachedTid 实例
    thread_local int t_cachedTid = 0;

    void cacheTid() {
        if (t_cachedTid == 0) {
            //通过linux系统调用获取当前线程pid
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}