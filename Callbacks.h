//
// Created by 27262 on 2024/6/22.
//

#ifndef MUDUO_CALLBACKS_H
#define MUDUO_CALLBACKS_H

#include <memory>
#include <functional>
#include "Timestamp.h"

class TcpConnection;

class Buffer;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;

using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
#endif //MUDUO_CALLBACKS_H
