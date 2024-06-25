//
// Created by 27262 on 2024/6/15.
//

#include "EpollPoller.h"
#include "Logger.h"
#include <cerrno>
#include <unistd.h>
#include <cassert>
#include <cstdlib>
#include <cstring>

constexpr int kNew = -1; //channel未添加到poller中, channel的index初始化也为-1
constexpr int kAdded = 1; //channel已添加到poller中
constexpr int kDeleted = 2; //channel已被删除
// EPOLL_CLOEXEC 可以让子进程调用exec()的时候关闭该fd,防止其被子进程继承
EpollPoller::EpollPoller(EventLoop *loop) : Poller(loop),
                                            epollfd_(epoll_create1(EPOLL_CLOEXEC)),
                                            events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create: %d\n", errno);
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}

/*
 * EventLoop   Poller
 * ChannelList  ChannelMap fd->Channel*
 */
void EpollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();
    LOG_INFO("func = %s => fd = %d events = %d index = %d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    if (index == kNew || index == kDeleted) {
        int fd = channel->fd();
        if (index == kNew) { //从来没有添加到Poller中
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else { //index = kDeleted,已添加过但是删除了
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else { //index == kAdded
        if (channel->isNoneEvent()) { //channel已经对任何事件都不感兴趣了
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    LOG_INFO("func = %s => fd = %d\n", __FUNCTION__, fd);
    int index = channel->index();
    channels_.erase(fd);
    // 如果此channel加入到了红黑树里面,调用epoll_ctl函数
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::update(int operation, Channel *channel) const {
    epoll_event event{};
    memset(&event, 0, sizeof(event));
    int fd = channel->fd();
    event.events = channel->events();
    //epoll_event的data成员是个联合体
    event.data.ptr = channel;
    if (epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        } else {
            //添加和修改的错误是致命的
            LOG_FATAL("epoll_ctl error: %d\n", errno);
        }
    }
}

//调用epoll_wait,把得到的epoll_events传入到EventLoop的activeChannels
Timestamp EpollPoller::poll(int timeoutMs, Poller::ChannelList *activeChannels) {
    // 这个方法会被频繁调用,建议使用LOG_DEBUG
    LOG_INFO("func = %s => fd total count: %zu\n", __FUNCTION__, channels_.size());
    // &*vec.begin()得到了vector底层元素数组的首地址,返回值是有几个被监听的fd发生了事件
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno; //一个线程是一个EventLoop,会多线程访问errno这个宏
    Timestamp now{Timestamp::now()};
    if (numEvents > 0) {
        LOG_INFO("%d events happened\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        //vector扩容,因为使用的是LT模式,由于events_大小受限制这次循环没有处理的事件下次还能触发
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_INFO("%s timeout \n", __FUNCTION__);
    } else {
        //系统调用被中断
        if (saveErrno != EINTR) {
            LOG_ERROR("EPollPoller::poll() err!\n");
        }
    }
    return now;
}

void EpollPoller::fillActiveChannels(int numEvents, Poller::ChannelList *activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        auto *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        //EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
        activeChannels->push_back(channel);
    }
}
