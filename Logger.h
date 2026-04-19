#pragma once


#include <string>

#include "noncopyable.h"

// 全局日志开关，默认开启
extern bool g_logEnable;

// LOG_INFO("%s %d",arg1, arg2)
#define LOG_INFO(logmsgFormat, ...)\
    do\
    {\
        if(g_logEnable){\
            Logger &logger = Logger::instance();\
            logger.setLogLevel(INFO);\
            char buf[1024]={0};\
            snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
            logger.log(buf);\
        }\
    }while(0)

#define LOG_ERROR(logmsgFormat, ...)\
    do\
    {\
        if(g_logEnable){\
            Logger &logger = Logger::instance();\
            logger.setLogLevel(ERROR);\
            char buf[1024]={0};\
            snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
            logger.log(buf);\
        }\
    }while(0)

#define LOG_FATAL(logmsgFormat, ...)\
    do\
    {\
        if(g_logEnable){\
            Logger &logger = Logger::instance();\
            logger.setLogLevel(FATAL);\
            char buf[1024]={0};\
            snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
            logger.log(buf);\
        }\
        exit(-1);\
    }while(0)

//debug信息比较多，打印出来很多影响查看正常流程，降低效率
//所以默认是关闭的
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)\
    do\
    {\
        if(g_logEnable){\
            Logger &logger = Logger::instance();\
            logger.setLogLevel(DEBUG);\
            char buf[1024]={0};\
            snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);\
            logger.log(buf);\
        }\
    }while(0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif
//定义日志的级别 INFO打印重要的流程信息
//ERROR错误但是不影响运行 FATAL毁灭性错误 DEBUG
enum LogLevel
{
    INFO,//普通信息
    ERROR,//错误信息
    FATAL,//core崩溃信息
    DEBUG,//调试信息
};

//输出一个日志类
class Logger : noncopyable
{
public:
    //获取日志唯一的实例对象
    static Logger& instance();
    //设置日志级别
    void setLogLevel(int level);
    //写日志
    void log(std:: string msg);
private:
    int logLevel_;//_在后面为了与系统变量不产生冲突，因为系统变量的_一般在前面

    Logger(){}
};