//
// Created by 27262 on 2024/6/21.
//

#ifndef MUDUO_EVENTLOOPTHREADPOOL_H
#define MUDUO_EVENTLOOPTHREADPOOL_H

#include "nocopyable.h"
#include <vector>
#include <string>
#include "EventLoopThread.h"
#include <functional>

class EventLoopThreadPool : nocopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseloop, const std::string &nameArg);

    ~EventLoopThreadPool(){}

    void setThreadNum(int numThreads) {
        numThreads_ = numThreads;
    }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果工作在多线程中,baseLoop_会默认以轮询的方式分配channel给subLoop
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }

    const std::string &getName() const { return name_; }

private:
    EventLoop *baseLoop_{};
    std::string name_{};
    bool started_{};
    int numThreads_{};
    int next_{};
    std::vector<std::unique_ptr<EventLoopThread>> threads_{};
    std::vector<EventLoop *> loops_{};

};


#endif //MUDUO_EVENTLOOPTHREADPOOL_H
