//
// Created by 27262 on 2024/6/13.
//

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

Channel::Channel(EventLoop *loop, int fd) : loop_{loop}, fd_{fd} {

}

Channel::~Channel() {

}

void Channel::tie(const std::shared_ptr<void> &ptr) {
    tie_ = ptr;
    tied_ = true;
}

void Channel::update() {
    //通过channel所属的eventloop,调用poller相应的方法,注册fd的events事件
}

//在channel所属的EventLoop中,把当前的channel删除掉
//@todo
void Channel::remove() {

}

void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        } else {
            handleEventWithGuard(receiveTime);
        }
    }
}

//根据Poller通知的channel发生的具体时间,由channel负责具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    //发生异常了
    LOG_INFO("channel handleEvent revents: %d", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        if (closeCallback_)
            closeCallback_();

    if (revents_ & EPOLLERR)
        if (errorCallback_)
            errorCallback_();

    if (events_ & (EPOLLIN | EPOLLPRI))
        if (readCallback_)
            readCallback_(receiveTime);
    if (events_ & EPOLLOUT)
        if (writeCallback_)
            writeCallback_();
}

