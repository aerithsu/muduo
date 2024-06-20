//
// Created by 27262 on 2024/6/15.
//
#include "Poller.h"
#include <cstdlib>
#include "EpollPoller.h"

// 不要在Poller基类的cpp文件里面引入派生类的头文件,新建一个文件来解耦合
Poller *Poller::newDefaultPoll(EventLoop *loop) {
    if (getenv("MUDUO_USE_POLL")) {
        return nullptr; //生成poll的实例
    } else {
        return new EpollPoller(loop); //生成epoll的实例
    }
}
