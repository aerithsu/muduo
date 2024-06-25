//
// Created by 27262 on 2024/6/22.
//
#include "Logger.h"
#include "TcpConnection.h"

static inline EventLoop *checkLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s: %s: %d mainlooop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr) :
        loop_(checkLoopNotNull(loop)),
        name_(name),
        state_(kConnecting),//正在连接
        socket_(std::make_unique<Socket>(sockfd)),
        channel_(std::make_unique<Channel>(loop, sockfd)),
        localAddr_(localAddr),
        peerAddr_(peerAddr),
        highWaterMark_(64 * 1024 * 1024) // 64M,防止发送过快而对吗接收太慢
{
    using namespace std::placeholders;
    // 给channel设置相应的回调函数,poller给channel通知感兴趣的事件发生了,channel会调用相应的回调
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::closeCallback_, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    socket_->setKeepAlive(true);
    LOG_INFO("TcpConnection::ctor[%s] at fd = %d\n", name.c_str(), sockfd);
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::dtor[%s] at fd = %d state = %d\n", name_.c_str(), channel_->fd(), (int) state_);
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if (n > 0) {
        // 已建立连接的用户有可读事件发生了,调用用户定义的回调
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead error\n");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    // 唤醒loop_对应的thread,执行回调
                    // 这里为什么queueInLoop呢?
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite() error\n");
        }
    } else {
        LOG_ERROR("TcpConnection fd = %d is down, no more writing \n", channel_->fd());
    }
}

// Poller => Channel::closeCallback => TcpConnection::handleClose
void TcpConnection::handleClose() {
    LOG_INFO("fd = %d state = %d \n", channel_->fd(), static_cast<int>(state_));
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr{shared_from_this()};
    if (connectionCallback_)
        connectionCallback_(connPtr); // 执行连接关闭的回调
    if (closeCallback_)
        closeCallback_(connPtr); // 关闭连接的回调,执行的是TcpServer::removeConnection回调方法
}

void TcpConnection::setState(TcpConnection::StateE state) {
    state_ = state;
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError() name : %s - SO_ERROR: %d \n", name_.c_str(), err);
}



// send的时候都是把数据转化为字符串

void TcpConnection::send(const std::string &buf) {
    // 只有建立连接的状态才能发送数据
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf.c_str(), buf.size());

        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

// 发送数据,应用写的快(非阻塞IO),而内核发送的慢,需要把待发送的数据写入缓冲区,而且设置了高水位回调
void TcpConnection::sendInLoop(const void *data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    // 之前已经调用过connection的shutdown了,不能send了
    if (state_ == kDisconnected) {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }
    // 表示channel_第一次开始写数据而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                // 数据全部发送完成,就不用给chanel设置EPOLLOUT事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else { // nwrote == 0
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop error\n");
                if (errno == EPIPE || errno == ECONNRESET) { // SIGPIPE RESET
                    faultError = true;
                }
            }
        }
    }
    // 未发生错误,且当前这次write并没有把数据全部发送出去,剩余数据需要保存到缓冲区里面
    // 然后给channel_注册EPOLLOUT事件,poller发现tcp的发送缓冲区有空间,会通知相应的sock(channel)调用handleWrite回调
    // 也就是调用TcpConnection::handleWrite方法,把发送缓冲区的数据全部发送完成
    if (!faultError && remaining > 0) {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting(); //这里一定要注册channel的写事件,否则Poller不会给channel通知EPOLLOUT
        }
    }
}

// 连接建立,由TcpServer调用
void TcpConnection::connectEstablished() {
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    // 新连接建立,执行回调
    connectionCallback_(shared_from_this());
}

//连接销毁,由TcpServer调用
void TcpConnection::connectDestroy() {
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel所有的感兴趣的事件从Poller中删除
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); //Channel从Poller移除

}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    // 说明当前输出缓冲区数据已经全部发送完成
    // 否则会一直写完再调用shutdownInLoop方法
    if (!channel_->isWriting()) {
        socket_->shutdownWrite(); // 关闭写端,Poller给Chanel通知回调handleClose
    }

}

