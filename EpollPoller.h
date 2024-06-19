//
// Created by 27262 on 2024/6/15.
//

#ifndef MUDUO_EPOLLPOLLER_H
#define MUDUO_EPOLLPOLLER_H

#include "Poller.h"
#include <vector>
#include <sys/epoll.h>

/*
 * epoll的使用
 * epoll_crate
 * epoll_ctl add/modify/delete
 * epoll_wait
 */
class EpollPoller : public Poller {
public:
    explicit EpollPoller(EventLoop *loop);

    Timestamp poll(int timeoutMs, Poller::ChannelList *activeChannels) override;

    void updateChannel(Channel *chanel) override;

    void removeChannel(Channel *channel) override;

    virtual ~EpollPoller();

private:
    static const int kInitEventListSize = 16;

    //填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    //更新channel
    void update(int operation, Channel *channel) const;

    using EventList = std::vector<epoll_event>;

    int epollfd_{};
    EventList events_{};
};


#endif //MUDUO_EPOLLPOLLER_H
