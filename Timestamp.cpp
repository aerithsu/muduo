//
// Created by 27262 on 2024/6/12.
//

#include "Timestamp.h"
#include <time.h>

Timestamp::Timestamp() = default;

Timestamp Timestamp::now() {
    return Timestamp(time(nullptr));
}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_{microSecondsSinceEpoch} {

}

//这个函数里面维护了一个静态的tm结构体,多线程访问可能会有竞争问题
std::string Timestamp::toString() const {
    char buf[128]{};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec
    );
    return buf;
}
