//
// Created by 27262 on 2024/6/21.
//

#ifndef MUDUO_THREAD_H
#define MUDUO_THREAD_H

#include "Noncopyable.h"
#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>

class Thread : Noncopyable {
public:
    //如果想要参数可以直接使用std::bind()
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, std::string name = "");

    ~Thread();

    void start();

    void join();

    bool started() const {
        return started_;
    }

    pid_t tid() const {
        return tid_;
    }

    const std::string &name() const {
        return name_;
    }

    static int numCreated() {
        return numCreated_;
    }

private:
    void setDefaultName();

    bool started_{};
    bool joined_{};
    //一旦定义对象就会直接启动线程
    //std::thread thread_{};
    std::unique_ptr<std::thread> thread_{};
    pid_t tid_{};
    ThreadFunc func_{};

    std::string name_{}; //Debug使用的

    //记录创建了几个线程的静态成员
    static std::atomic<int> numCreated_;
};

#endif //MUDUO_THREAD_H
