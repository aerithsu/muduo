//
// Created by 27262 on 2024/6/12.
//

#ifndef MUDUO_TIMESTAMP_H
#define MUDUO_TIMESTAMP_H

#include <stdint.h>
#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp();

    [[nodiscard]] std::string toString() const ;

    static Timestamp now();

    explicit Timestamp(int64_t
              microSecondsSinceEpoch);

private:
    int64_t microSecondsSinceEpoch_{};
};


#endif //MUDUO_TIMESTAMP_H
