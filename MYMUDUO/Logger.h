#pragma once

#include <string>

#include "noncopyable.h"

#define LOG_INFO(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024]; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0); 

#define LOG_ERROR(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024]; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0); 

#define LOG_FATAL(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024]; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while (0); 

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024]; \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while (0); 
#else
   #define LOG_DEBUG(LogmsgFormat, ...)
#endif 

enum logLevel
{
    INFO,  //普通信息
    ERROR, //错误信息
    FATAL, //严重错误
    DEBUG, //调试信息
};

/*
    饿汉式实现单利
    构造函数私有化，在类的内部创建实例，提供获取唯一实例的方法
*/

class Logger : noncopyable
{
private:
    //日志级别
    int logLevel_;
public:
    //获取日志唯一的实例对象
    static Logger& instance();
    //设置日志级别
    void setLogLevel(int Level);
    //写日志
    void log(std::string msg);
};
