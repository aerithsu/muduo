//
// Created by 27262 on 2024/6/13.
//

#ifndef MUDUO_EVENTLOOP_H
#define MUDUO_EVENTLOOP_H

#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include "nocopyable.h"
#include "Channel.h"
#include "EpollPoller.h"
#include "Timestamp.h"
#include "CurrentThread.h"

//事件循环类,主要包含了Channel和Poller(epoll的抽象)
class EventLoop : nocopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();

    ~EventLoop();

    //开启事件循环
    void loop();

    //退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    //在当前loop中执行cb
    void runInLoop(Functor cb);

    //把cb放在队列中,唤醒loop所在的线程,执行cb
    void queueInLoop(Functor cb);

    void wakeup();

    // EventLoop的方法 调用 poller的方法
    void updateChannel(Channel *channel);

    void removeChannel(Channel *channel);

    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里
    bool isInloopThread() const {
        return threadId_ == CurrentThread::tid();
    }

private:
    void handleRead(); //wakeup

    void doPendingFunctor(); //执行回调

private:

    using ChannelList = std::vector<Channel *>;

    std::atomic<bool> looping_{false};
    std::atomic<bool> quit_{false}; // 标志退出loop循环

    const pid_t threadId_{}; //记录当前loop所在的线程ID,用来判断EventLoop在不在创建它的线程里

    Timestamp pollReturnTime_{}; //记录poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_{};

    // linux eventfd()机制
    int wakeupFd_{}; // 当mainLoop获取一个新用户的channel通过轮询算法选择一个subLoop,通过该成员来进行唤醒
    std::unique_ptr<Channel> wakeupChannel_{};

    ChannelList activeChannels_{};
    //Channel* currentActiveChannel_{};

    std::atomic<bool> callingPendingFunctors{false}; //标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_{}; //存储loop需要执行所有的回调操作
    mutable std::mutex mutex_; //用来保护上面vector容器的线程安全操作
};


#endif //MUDUO_EVENTLOOP_H
