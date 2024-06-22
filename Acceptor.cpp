//
// Created by 27262 on 2024/6/21.
//

#include "Acceptor.h"

//static可以让此函数只在此文件有效
static int createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        LOG_FATAL("%s: %s: %d listen socket create error:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport) :
        loop_{loop}, acceptSocket_{createNonblocking()}, acceptChannel_(loop, acceptSocket_.fd()) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); //绑定套接字
    //TCPServer::start()进行listen
    //如果有新用户连接,要执行一个回调(connfd->channel->subloop),
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

//listenfd有事件发生了,就是有新用户连接了,此函数作为回调被调用
void Acceptor::handleRead() {
    InetAddress peerAddr{};
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else { //这种情况其实是出错了
            ::close(connfd);
        }
    } else {
        LOG_ERROR("%s: %s: %d accept error:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        //fd用完了
        if (errno == EMFILE) {
            //这种情况可以调整系统的fd最大数,或者增加更多主机成为一个分布式集群
            LOG_ERROR("%s: %s: %d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen() {
    listening_ = true;
    acceptSocket_.listen(); // listen
    acceptChannel_.enableReading(); //注册到Poller里
}
