//
// Created by 27262 on 2024/6/22.
//

#include "Buffer.h"
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

// Poller工作在LT模式,从fd上读取数据
// Buffer缓存区是有大小的,但是从fd读的时候确不知道tcp数据最终大小(流式数据)
ssize_t Buffer::readFd(int fd, int *saveErrno) {
    // 一次最多读64K bytes的数据
    char extraBuf[65536]{}; // 64K
    iovec vec[2];
    const size_t writeable = writeableBytes();
    // iov先填充Buffer,再填充栈上的extraBuf
    vec[0].iov_base = beginWrite() + writerIndex_;
    vec[0].iov_len = writeable;

    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof(extraBuf);
    const int iovcnt = writeable < sizeof(extraBuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt); // n是读取的字节数 第二个参数数组名本身就可以作为地址
    if (n < 0) {
        *saveErrno = errno;
    } else if (n <= writeable) { //Buffer的可写缓存区已经可以存储读出来的数据了
        writerIndex_ += n;
    } else { //extraBuf里面也写入了数据
        writerIndex_ = buffer_.size();
        append(extraBuf, n - writeable); // 还需要扩容n - writable,再写入这些数据
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}
