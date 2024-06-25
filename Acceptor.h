//
// Created by 27262 on 2024/6/21.
//

#ifndef MUDUO_ACCEPTOR_H
#define MUDUO_ACCEPTOR_H

#include <utility>

#include "EventLoopThreadPool.h"
#include "Socket.h"

// Acceptor是运行在baseLoop里面的
class Acceptor : Noncopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &addr)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);

    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb) {
        newConnectionCallback_ = std::move(cb);
    }

    void listen() ;
    bool listening() const{return listening_;}
private:
    void handleRead();

    EventLoop *loop_; //Acceptor用的用户定义的baseLoop,也称作mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    // 轮询找到subLoop,唤醒,分发当前新客户端的channel
    NewConnectionCallback newConnectionCallback_;
    bool listening_{};
};


#endif //MUDUO_ACCEPTOR_H
