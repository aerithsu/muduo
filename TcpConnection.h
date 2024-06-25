//
// Created by 27262 on 2024/6/22.
//

#ifndef MUDUO_TCPCONNECTION_H
#define MUDUO_TCPCONNECTION_H

#include "Noncopyable.h"
#include "Channel.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Buffer.h"
#include <memory>
#include <atomic>
#include "Callbacks.h"

/*
 *  TcpServer => Acceptor => 有一个新用户连接,通过accept拿到connfd
 *  TcpConnection 设置回调 => channel => poller => channel回调操作
 */
class TcpConnection : Noncopyable, public std::enable_shared_from_this<TcpConnection> {
private:
    enum StateE {
        kDisconnected, kConnecting, kConnected, kDisconnecting
    };
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr
    );

    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }

    const std::string &name() const { return name_; }

    const InetAddress &localAddress() const { return localAddr_; }

    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string& buf);

    // 关闭连接
    void shutdown();

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }

    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }

    void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark) {
        highWaterMarkCallback_ = std::move(cb);
        highWaterMark_ = highWaterMark;
    }

    void setCloseCallback(CloseCallback cb) {
        closeCallback_ = std::move(cb);
    }

    void connectEstablished();

    void connectDestroy();

private:
    void handleRead(Timestamp receiveTime);

    void handleWrite();

    void handleClose();

    void handleError();

    void sendInLoop(const void *message, size_t len);

    void shutdownInLoop();

    void setState(StateE state);


private:

    EventLoop *loop_{}; // 绝对不是baseLoop,因为TcpConnection都是在sub loop里面管理的
    const std::string name_{};
    std::atomic<int> state_;
    bool reading_{};

    // 这里和Acceptor类似,只不过Acceptor在main loop,这个在sub loop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_{};
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};


#endif //MUDUO_TCPCONNECTION_H
