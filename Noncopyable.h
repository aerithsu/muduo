//
// Created by 27262 on 2024/6/12.
//

#ifndef MUDUO_NONCOPYABLE_H
#define MUDUO_NONCOPYABLE_H

/*
 * noncopyable被继承以后,派生类对象可以正常的构造和析构,但是无法进行拷贝操作
 */

class Noncopyable {
public:
    Noncopyable(const Noncopyable &) = delete;
    //其实这里返回值写void也可以
    Noncopyable &operator=(const Noncopyable &) = delete;

protected:
    ~Noncopyable() = default;

    Noncopyable() = default;
};


#endif //MUDUO_NONCOPYABLE_H
