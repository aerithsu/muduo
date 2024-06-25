//
// Created by 27262 on 2024/6/22.
//

#ifndef MUDUO_BUFFER_H
#define MUDUO_BUFFER_H

#include <vector>
#include <string>
#include <algorithm>

class Buffer {
public:
    static constexpr std::size_t kCheapPrepend = 8;
    static constexpr std::size_t kInitialSize = 1024;

    explicit Buffer(std::size_t initialSize = kInitialSize) : buffer_(kCheapPrepend + initialSize),
                                                              readerIndex_{kCheapPrepend},
                                                              writerIndex_{kCheapPrepend} {}

    std::size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    std::size_t writeableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    std::size_t prependableBytes() const {
        return readerIndex_;
    }

    //返回buffer中可读数据的起始地址
    const char *peek() const {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    void retrieve(std::size_t len) {
        if (len < readableBytes()) {
            readerIndex_ += len; //应用只读取了可读缓冲区的一部分(长度为len),更新reader_index
        } else { //len == readableBytes()
            retrieveAll();
        }
    }

    // 把OnMessage函数上报的Buffer数据,转成string类型返回
    void retrieveAll() {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString(std::size_t len) {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    void ensureWriteableBytes(std::size_t len) {
        if (writeableBytes() < len) {
            makeSpace(len);
        }
    }

    //把长度为len的的数据添加到buffer中
    void append(const char *data, size_t len) {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite() {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);

    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin() {
        return &*buffer_.begin();
    }

    const char *begin() const {
        return &*buffer_.cbegin();
    }

    // 扩容函数
    void makeSpace(std::size_t len) {
        if (writeableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {
            std::size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    std::size_t readerIndex_{};
    std::size_t writerIndex_{};
};


#endif //MUDUO_BUFFER_H
