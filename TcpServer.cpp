//
// Created by 27262 on 2024/6/13.
//

#include "TcpServer.h"
#include "Logger.h"

static inline EventLoop *checkLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s: %s: %d mainlooop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, std::string nameArg, TcpServer::Option option)
        : loop_{checkLoopNotNull(loop)},
          ipPort_(listenAddr.toIpPort()),
          name_(std::move(nameArg)),
          acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
          threadPool_(new EventLoopThreadPool(loop, name_)) {
    using namespace std::placeholders;
    //当有新用户连接时,执行对应回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

// 有一个新客户端的连接,会执行这个回调
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    EventLoop *ioLoop = threadPool_->getNextLoop(); // 轮询算法选择一个subLoop,来管理channel
    char buffer[64]{};
    snprintf(buffer, sizeof(buffer), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buffer;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n", name_.c_str(), connName.c_str(),
             peerAddr.toIpPort().c_str());
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local{};
    ::bzero(&local, sizeof(local));
    socklen_t addrLen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr *) &local, &addrLen) < 0) {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr{local};

    //根据连接成功的sockfd,创建TcpConnection连接对象
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(ioLoop, connName, sockfd, localAddr, peerAddr);
    connections_[connName] = conn;

    // 下面的回调都是用户设置给TcpServer的=>TcpConnection=>Channel=>Poller=>notify channel调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroy, conn));
}

void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    //防止一个TcpServer被启动多次
    if (started_++ == 0) {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

TcpServer::~TcpServer() {
    for (auto &item: connections_) {
        // 因为调用TcpConnection::connectDestroy是异步调用的,所有要移动这个智能指针给conn变量,确保对应对象不会过期
        TcpConnectionPtr conn{std::move(item.second)};
        // 销毁连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroy, conn));
    }
}

