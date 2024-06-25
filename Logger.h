//
// Created by 27262 on 2024/6/12.
//

#ifndef MUDUO_LOGGER_H
#define MUDUO_LOGGER_H

#include <string>

#include "Noncopyable.h"

//定义日志的级别 INFO ERROR FATAL DEBUG
enum class LogLevel {
    INFO, //普通信息
    ERROR, // 错误信息
    FATAL, //core信息
    DEBUG, //调试信息
};
// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
         Logger& logger = Logger::getInstance();\
         logger.setLogLevel(LogLevel::INFO);\
         char buf[1024]{};\
         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
         logger.log(buf);\
    } while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
         Logger& logger = Logger::getInstance();\
         logger.setLogLevel(LogLevel::ERROR);\
         char buf[1024]{};\
         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
         logger.log(buf);\
    } while(0)
#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
         Logger& logger = Logger::getInstance(); \
         logger.setLogLevel(LogLevel::FATAL);\
         char buf[1024]{};\
         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
         logger.log(buf);            \
         exit(-1); \
    } while(0)
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
         Logger& logger = Logger::getInstance();\
         logger.setLogLevel(LogLevel::INFO::DEBUG);\
         char buf[1024]{};\
         snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
         logger.log(buf);\
    } while(0)

#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

//单例
class Logger : Noncopyable {
public:
    // 获取唯一的Logger实例对象
    static Logger &getInstance();


    //设置日志级别
    void setLogLevel(LogLevel level);

    //写日志
    void log(const std::string &msg);

private:
    LogLevel logLevel_;

    Logger() {}
};


#endif //MUDUO_LOGGER_H
