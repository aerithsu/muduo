//
// Created by 27262 on 2024/6/13.
//

#ifndef MUDUO_CHANNEL_H
#define MUDUO_CHANNEL_H

#include "nocopyable.h"
#include <functional>
#include <memory>
#include "Timestamp.h"
#include "sys/epoll.h"

/*
 * Channel理解为通道,封装了sockfd和其感兴趣的event,如EPOLLIN,EPOLLOUT事件
 */
class EventLoop; //类型前置声明,因为没用到具体实现,所以只要声明不引入头文件,减少头文件的引入

class Channel : nocopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);

    ~Channel();

    //这里需要知道Timestamp的具体大小,要引入头文件
    //fd得到poller通知以后,处理事件方法
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    void setReadCallback(ReadEventCallback cb) {
        readCallback_ = std::move(cb);
    }

    void setWriteCallback(EventCallback cb) {
        writeCallback_ = std::move(cb);
    }

    void setCloseCallback(EventCallback cb) {
        closeCallback_ = std::move(cb);
    }

    void setErrorCallback(EventCallback cb) {
        errorCallback_ = std::move(cb);
    }

    //防止当channel被手动remove掉,channel还在执行回调操作
    void tie(const std::shared_ptr<void> &ptr);

    int fd() const { return fd_; }

    int events() const { return events_; }

    int set_revents(int revt) {
        revents_ = revt;
    }


    //设置fd相应的事件状态
    void enableReading() {
        events_ |= kReadEvent;
        update();
    }

    void disableReading() {
        events_ &= ~kReadEvent;
        update();
    }

    void enableWriting() {
        events_ |= kWriteEvent;
        update();
    }

    void disableWriting() {
        events_ &= ~kWriteEvent;
        update();
    }

    void disableAll() {
        events_ = kNoneEvent;
        update();
    }

    //返回fd当前的事件状态
    bool isNoneEvent() const {
        return events_ == kNoneEvent;
    }

    bool isWriting() const {
        return events_ & kWriteEvent;
    }

    bool isReading() const {
        return events_ & kReadEvent;
    }

    int index() const { return index_; }

    void set_index(int idx) {
        index_ = idx;
    }

    // one loop per thread
    // 当前Channel属于哪个EventLoop,一个EventLoop可以包括多个Channel

    EventLoop *ownerLoop() {
        return loop_;
    }

    void remove();

private:
    //static的const变量可以直接定义,因为可以看作字面常量直接替换
    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;

    EventLoop *loop_{}; //事件循环
    const int fd_{}; //Poller监听的对象
    int events_{}; //注册fd感兴趣的事件
    int revents_{}; //poller返回的具体发生的事件
    int index_{-1};

    std::weak_ptr<void> tie_; //绑定自己,可以用shared_from_this替代
    bool tied_{};
    //因为channel通道里面能获知fd最终发生的具体事件revents,所有它们负责具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
    //当改变channel所表示的fd的events事件后,update负责在poller里更改fd相应事件的epoll_cnt
    void update();

    void handleEventWithGuard(Timestamp receiveTime);
};


#endif //MUDUO_CHANNEL_H
