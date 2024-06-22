//
// Created by 27262 on 2024/6/13.
//

#ifndef MUDUO_TCPSERVER_H
#define MUDUO_TCPSERVER_H

#include "nocopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include <unordered_map>

class TcpServer : nocopyable {
public:
    using ThreaInitCallback = std::function<void(EventLoop *)>;

private:
    EventLoop *loop_{}; //baseLoop用户定义的loop
    const std::string ipPort_{};
    const std::string name_{};
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread
    
};


#endif //MUDUO_TCPSERVER_H
