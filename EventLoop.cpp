//
// Created by 27262 on 2024/6/13.
//

#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

//防止一个线程创建多个EventLoop
thread_local EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000; // 10 seconds

// 创建wakeupfd,用来唤醒subReactor处理新来的channel
int createEventFd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error: %d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop() :
        threadId_(CurrentThread::tid()),
        poller_(Poller::newDefaultPoll(this)),
        wakeupFd_(createEventFd()),
        wakeupChannel_(new Channel(this, wakeupFd_)) {
    LOG_INFO("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists int the thread %d \n", t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    //可以让function<void(Timestamp)>类型接收std::bind(&EventLoop::handleRead, this)
    //wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->setReadCallback([this](Timestamp) { handleRead(); });
    // 每一个EventLoop都将鉴定wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof one) {
        LOG_ERROR("Event:oop: handleRead() reads %zd bytes instead of 8", n);
    }
}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);
    while (!quit_) {
        activeChannels_.clear();
        // 监听两类fd, 一种是client的fd,一种是wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (auto channel: activeChannels_) {
            //Poller监听哪些channel发生事件了,然后上报给EventLoop,通知channel调用用户定义的回调处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前EventLoop事件循环需要处理的回调操作
        /*
         * IO线程 mainLoop accept fd <== channel sub loop
         * mainLoop 事先注册一个回调cb（需要sub loop来执行）    wakeup sub loop后，执行下面的方法，执行之前mainloop注册的cb操作
         */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
    looping_ = false;
}

// 1.loop在自己的线程中调用quit
// 2.如果是在其他线程中调用了quit() 如在一个subLoop(worker)调用了mainLoop的quit
//
void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        //唤醒一下别的EventLoop的线程
        wakeup();
    }
}

void EventLoop::runInLoop(const EventLoop::Functor& cb) {
    if (isInLoopThread()) { //在当前的loop线程中,执行cb
        cb();
    } else { //在非当前loop线程中执行cb,就需要唤醒loop所在线程,执行cb
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(EventLoop::Functor cb) {
    {
        std::lock_guard lock{mutex_};
        pendingFunctors_.emplace_back(std::move(cb));
    }
    //第二个条件是callingPendingFunctors == true,代表正在执行回调,但是loop又有了新的回调
    //此时运行到下一轮循环还会被阻塞到poll调用,故需要wakeup一下,继续执行回调
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup(); //唤醒loop所在线程
    }
}

// 向wakeupfd写一个数据,wakeupChannel就发生读事件,当前loop线程就会被唤醒
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8", n);
    }
}

//channel通过EventLoop间接和poller沟通
void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    //把pendingFunctors_里的回调函数交换到functors里面,这样可以减小加锁的时间
    //防止后续事件长时间无法加入pendingFunctors_中
    {
        std::lock_guard lock{mutex_};
        functors.swap(pendingFunctors_);
    }
    for (const auto &functor: functors) {
        functor(); //执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}
