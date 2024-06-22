//
// Created by 27262 on 2024/6/21.
//

#ifndef MUDUO_EVENTLOOPTHREAD_H
#define MUDUO_EVENTLOOPTHREAD_H

#include "Thread.h"
#include <condition_variable>
#include "nocopyable.h"
#include "EventLoop.h"

class EventLoopThread : nocopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    explicit EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback{}, const std::string &name = "");

    ~EventLoopThread();

    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *loop_{};
    bool exiting_{};
    Thread thread_;
    std::mutex mtx_{};
    std::condition_variable cond_{};
    ThreadInitCallback callback_{};
};


#endif //MUDUO_EVENTLOOPTHREAD_H
