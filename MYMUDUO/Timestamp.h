#pragma once

#include <iostream>
#include <string>

class Timestamp
{
private:
    int64_t  nowTime_;
public:
    Timestamp();
    explicit Timestamp(int64_t nowTime);
    static Timestamp now();
    std::string toString() const;
};
