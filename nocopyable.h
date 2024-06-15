//
// Created by 27262 on 2024/6/12.
//

#ifndef MUDUO_NOCOPYABLE_H
#define MUDUO_NOCOPYABLE_H

/*
 * nocopyable被继承以后,派生类对象可以正常的构造和析构,但是无法进行拷贝操作
 */

class nocopyable {
public:
    nocopyable(const nocopyable &) = delete;
    //其实这里返回值写void也可以
    nocopyable &operator=(const nocopyable &) = delete;

protected:
    ~nocopyable() = default;

    nocopyable() = default;
};


#endif //MUDUO_NOCOPYABLE_H
