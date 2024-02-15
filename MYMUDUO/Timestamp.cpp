#include <ctime>

#include "Timestamp.h"

Timestamp::Timestamp() : nowTime_(0) {};

Timestamp::Timestamp(int64_t nowTime) : nowTime_(nowTime) {};

Timestamp Timestamp::now()
{
    return Timestamp(time(nullptr));
}

std::string Timestamp::toString() const 
{
    char buf[128];
    strftime(buf, 128, "%Y/%m/%d %H:%M:%S", localtime(&nowTime_));
    return buf;
}