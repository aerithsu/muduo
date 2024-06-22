//
// Created by 27262 on 2024/6/21.
//
#include "Thread.h"
#include "CurrentThread.h"
#include <utility>
#include <semaphore.h>

std::atomic<int> Thread::numCreated_ = 0;

Thread::Thread(std::function<void()> f, std::string name) : func_(std::move(f)),
                                                            name_(std::move(name)) {
    setDefaultName();
}

void Thread::setDefaultName() {
    int num = ++numCreated_;
    if (name_.empty()) {
        char buf[32]{};
        snprintf(buf, sizeof buf, "Thread: %d", num);
        name_ = buf;
    }
}

Thread::~Thread() {
    //该类被析构时线程已启动如果没有被join,需要detach()来让其系统回收线程资源
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start() {
    //一个Thread对象记录一个新线程的详细信息
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    thread_ = std::make_unique<std::thread>([&]() {
        //获取线程tid值
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_(); //开启一个新线程,专门执行该线程函数
    });
    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);
}

void Thread::join() {
    joined_ = true;
    thread_->join();
}

