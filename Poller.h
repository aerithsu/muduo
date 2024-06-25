//
// Created by 27262 on 2024/6/15.
//

#ifndef MUDUO_POLLER_H
#define MUDUO_POLLER_H

#include <sys/epoll.h>
#include "Noncopyable.h"
#include <vector>
#include "Channel.h"
#include <unordered_map>
#include "Timestamp.h"

// 多路事件分发器的核心IO复用模块
class Poller : Noncopyable {
public:
    using ChannelList = std::vector<Channel *>;

    explicit Poller(EventLoop *loop) : ownerloop_{loop} {}

    virtual  ~Poller() = default;

    //给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

    virtual void updateChannel(Channel *chanel) = 0;

    virtual void removeChannel(Channel *channel) = 0;

    //判断参数channel是否在当前Poller中
    bool hasChannel(Channel *channel) const {
        auto it = channels_.find(channel->fd());
        return it != channels_.end() && it->second == channel;
    }

    // EventLoop可以通过该接口获取默认的IO复用具体实现
    static Poller *newDefaultPoll(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    EventLoop *ownerloop_; // Poller所属的事件循环
};


#endif //MUDUO_POLLER_H
