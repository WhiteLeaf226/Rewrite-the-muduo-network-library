#pragma once

/**
 * 派生类对象无法拷贝构造和拷贝赋值
 * 派生类对象可以正常构造析构
 */
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;    
    ~noncopyable() = default;
};