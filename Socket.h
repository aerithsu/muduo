//
// Created by 27262 on 2024/6/21.
//

#ifndef MUDUO_SOCKET_H
#define MUDUO_SOCKET_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "nocopyable.h"
#include "InetAddress.h"
#include "Logger.h"
#include <cstring>
#include <netinet/tcp.h> // For TCP_NODELAY
#include <netinet/in.h>  // For IPPROTO_TCP

class Socket : nocopyable {
public:
    //创建完socket传入这个构造函数构造Socket类
    explicit Socket(int sockfd) : sockfd_(sockfd) {

    }

    ~Socket() {
        ::close(sockfd_);
    }

    int fd() const { return sockfd_; }

    void bindAddress(const InetAddress &localAddr) {
        if (0 != ::bind(sockfd_, (sockaddr *) localAddr.getSockAddr(), sizeof(sockaddr_in))) {
            LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
        }
    }

    void listen() {
        if (0 != ::listen(sockfd_, 1024)) {
            LOG_FATAL("listen sockfd:%d \n", sockfd_);
        }
    }

    int accept(InetAddress *peerAddr) {
        sockaddr_in addr{};
        ::bzero(&addr, sizeof(addr));
        socklen_t len;
        int connfd = ::accept(sockfd_, (sockaddr *) &addr, &len);
        if (connfd >= 0) {
            peerAddr->setSockAddr(addr);
        }
        return connfd;
    }

    void shutdownWrite() {
        if (::shutdown(sockfd_, SHUT_WR) < 0) {
            LOG_FATAL("shutdown error : %d", sockfd_);
        }
    }


    void setTcpNoDelay(bool on) {
        int optval = on ? 1 : 0;
        ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
    }

    void setReuseAddr(bool on) {
        int optval = on ? 1 : 0;
        ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    }

    void setReusePort(bool on) {
        int optval = on ? 1 : 0;
        ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    }

    void setKeepAlive(bool on) {
        int optval = on ? 1 : 0;
        ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
    }

private:
    const int sockfd_{};
};


#endif //MUDUO_SOCKET_H
