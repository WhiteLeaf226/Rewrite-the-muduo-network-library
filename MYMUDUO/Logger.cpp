#include <iostream>

#include "Logger.h"

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int Level)
{
    logLevel_ = Level;
}

void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO:
        std::cout<<"[INFO]";
        break;
    case ERROR:
        std::cout<<"[ERROR]";
        break;
    case FATAL:
        std::cout<<"[FATAL]";
        break;
    case DEBUG:
        std::cout<<"[DEBUG]";
        break;    
    }

    std::cout<<"print time"<<" : "<<msg<<std::endl;
}