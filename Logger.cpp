//
// Created by 27262 on 2024/6/12.
//
#include "Timestamp.h"
#include "Logger.h"
#include <iostream>


//[级别信息] time : msg
void Logger::log(const std::string &msg) {
    switch (logLevel_) {
        case LogLevel::INFO:
            std::cout << "[INFO]";
            break;
        case LogLevel::ERROR:
            std::cout << "[ERROR]";
            break;
        case LogLevel::FATAL:
            std::cout << "[FATAL]";
            break;
        case LogLevel::DEBUG:
            std::cout << "[DEBUG]";
            break;
        default:
            break;
    }
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}

void Logger::setLogLevel(LogLevel level) {
    logLevel_ = level;
}

Logger &Logger::getInstance() {
    static Logger logger;
    return logger;
}