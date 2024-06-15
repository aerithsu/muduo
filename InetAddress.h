//
// Created by 27262 on 2024/6/13.
//

#ifndef MUDUO_INETADDRESS_H
#define MUDUO_INETADDRESS_H

#include <cstdint>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>

//这个类允许拷贝
class InetAddress {
public:
    explicit InetAddress(int16_t port = 0, const std::string &ip = "127.0.0.1");

    explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

    [[nodiscard]] std::string toIp() const;

    [[nodiscard]] std::string toIpPort() const;

    [[nodiscard]] uint16_t toPort() const;

    sockaddr_in *getSockAddr() {
        return &addr_;
    }

    [[nodiscard]] const sockaddr_in *getSockAddr() const {
        return &addr_;
    }

private:
    sockaddr_in addr_;

};


#endif //MUDUO_INETADDRESS_H
