#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"
#include "EventLoop.h"

/**
 * channel为管道，封装了sockfd和sockfd兴趣的事件
 * 并绑定了poller返回的具体事件 
 */
class Channel : noncopyable
{
private:
    static const int kNoneEvent;
    static const int KReadEvent;
    static const int kWriteEvent;

    //事件循环
    EventLoop *loop_;
    //poller监听的对象
    const int fd_;
    /**
     * 注册fd感兴趣的事件
     * 注册后事件对应位会被置1，否则为0(二进制)
     */
    int events_;
    /**
     * poller返回的具体发生的事件
     * 发生对应事件会使对应位被置1，否则为0(二进制)
     */
    int revents_;
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;
public:
    //事件和只读事件回调
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    //fd得到poller通知后，调用对应方法处理事件
    void handleEvent(Timestamp receiveTime);

    //设置回调操作  
    //ReadEventCallback创建cb相当于 void a(Tiestamp b);
    void setReadCallback(ReadEventCallback cb) {readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {errorCallback_ = std::move(cb);}

    //防止channel被手动remove后，channel还在执行回调
    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}

    /**
     * 启用/禁用某一事件，将其注册到poller中
     * 然后由poller去选择对应的事件处理器 
     */

    //启用读事件
    void enableReading() {events_ |= KReadEvent; update();}
    //禁用读事件
    void disableReading() {events_ &= ~KReadEvent; update();}
    //启用写事件
    void enableWriting() {events_ |= kWriteEvent; update();}
    //禁用写事件
    void disableWriting() {events_ &= ~kWriteEvent; update();}
    //禁用所有事件
    void disableAll() {events_ = kNoneEvent; update();};

    /*是否注册事件*/
    //没有感兴趣的事件
    bool isNoneEvent() const {return events_ == kNoneEvent;};
    //对写事件感兴趣
    bool isWriting() const {return events_ & kWriteEvent;}
    //对读时间感兴趣
    bool isReading() const {return events_ & KReadEvent;}

    int index() const {return index_;}
    void set_index(int idx) {index_ = idx;}

    EventLoop* ownerLoop() {return loop_;}
    void remove();

    
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    //Channel管道中可以得到fd发生的具体事件，然后执行对应的回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
