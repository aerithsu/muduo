//
// Created by 27262 on 2024/6/13.
//

#ifndef MUDUO_TCPSERVER_H
#define MUDUO_TCPSERVER_H

#include "Noncopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include <unordered_map>
#include "Callbacks.h"
#include "TcpConnection.h"

#include <atomic>

class TcpServer : Noncopyable {
public:
    using ThreaInitCallback = std::function<void(EventLoop *)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    //k开头代表常量或者枚举值
    enum Option {
        klNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr, std::string nameArg, Option option = klNoReusePort);

    ~TcpServer();

    void setThreadInitCallback(ThreaInitCallback cb) {
        threadInitCallback_ = std::move(cb);
    }

    void setConnectionCallback(ConnectionCallback cb) {
        connectionCallback_ = std::move(cb);
    }

    void setMessageCallback(MessageCallback cb) {
        messageCallback_ = std::move(cb);
    }

    void setWriteCompleteCall(WriteCompleteCallback cb) {
        writeCompleteCallback_ = std::move(cb);
    }

    //设置底层subLoop的个数
    void setThreadNum(int numThreads);

    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);

    void removeConnection(const TcpConnectionPtr &conn);

    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    EventLoop *loop_{}; //baseLoop用户定义的loop

    const std::string ipPort_{};
    const std::string name_{};

    std::unique_ptr<Acceptor> acceptor_; //运行在mainLoop,任务就是监听新事件连接

    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_{}; //有新连接时的回调
    MessageCallback messageCallback_{}; //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_{}; //消息发送完成以后的回调

    ThreaInitCallback threadInitCallback_{}; // loop线程初始化的回调
    std::atomic<int> started_;

    int nextConnId_{1}; // 只有一个线程使用了这个变量
    ConnectionMap connections_{}; //保存所有的连接
};


#endif //MUDUO_TCPSERVER_H
