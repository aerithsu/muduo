//
// Created by 27262 on 2024/6/21.
//

#include "EventLoopThreadPool.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseloop, const std::string &nameArg) :
        baseLoop_{baseloop}, name_{nameArg} {

}

void EventLoopThreadPool::start(const EventLoopThreadPool::ThreadInitCallback &cb) {
    started_ = true;
    for (int i = 0; i < numThreads_; ++i) {
        char buf[1024]{};
        snprintf(buf, sizeof(buf), "%s %d", name_.c_str(), i);
        auto p = new EventLoopThread(cb, buf);
        threads_.emplace_back(std::unique_ptr<EventLoopThread>(p));
        loops_.emplace_back(p->startLoop()); //底层创建线程,绑定一个新的EventLoop,并返回该loop的地址
    }
    // 整个服务端只有一个线程运行这baseLoop
    //std::function可以转化为bool类型,也可以和nullptr做比较,为空的时候转化为false,== nullptr
    if (numThreads_ == 0 && cb != nullptr) {
        cb(baseLoop_);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
    EventLoop *loop = baseLoop_;
    //如果不止只有一个baseLoop,round-robin,通过轮询获取下一个处理事件的loop
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) {
        return {baseLoop_};
    }
    return loops_;
}
