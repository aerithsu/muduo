//
// Created by 27262 on 2024/6/21.
//

#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(const EventLoopThread::ThreadInitCallback &cb, const std::string &name)
        : thread_(std::bind(&EventLoopThread::threadFunc, this), name), callback_(cb) {

}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop() {
    thread_.start(); //启动底层新的线程
    EventLoop *loop{};
    {
        std::unique_lock lock{mtx_};
        cond_.wait(lock, [&]() { return loop != nullptr; });
        loop = loop_;
    }
    return loop;
}

// 下面这个方法是在单独的新线程里面运行的
void EventLoopThread::threadFunc() {
    EventLoop tloop; //创建一个独立的eventLoop,和上面的线程是一一对应的,one loop per thread
    if (callback_) {
        callback_(&tloop);
    }
    {
        std::unique_lock lock{mtx_};
        loop_ = &tloop;
        cond_.notify_one();
    }
    tloop.loop(); //调用Poller::poll()
    //执行到下面代码说明服务器程序已经关闭掉了
    std::lock_guard lock{mtx_};
    loop_ = nullptr;
}
