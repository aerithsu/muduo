//
// Created by 27262 on 2024/6/19.
//

#ifndef MUDUO_CURRENTTHREAD_H
#define MUDUO_CURRENTTHREAD_H

#include <pthread.h>

namespace CurrentThread {
    //extern和thread_local确保了变量可以在多个文件之间共享声明而在单个位置定义，同时为每个线程提供唯一的实例
    extern thread_local int t_cachedTid;

    void cacheTid();

    inline int tid() {
        if (t_cachedTid == 0)[[unlikely]] {
            cacheTid();
        }
        return t_cachedTid;
    }
}


#endif //MUDUO_CURRENTTHREAD_H
