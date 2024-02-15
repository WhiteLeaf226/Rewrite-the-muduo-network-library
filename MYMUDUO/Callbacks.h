#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

//TcpConnectionPtr相当于指向TcpConnection的shared_ptr指针
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;

using MessageCallback = std::function<void (const TcpConnectionPtr&,
                                      Buffer*, 
                                      Timestamp)>;

using HighWaterMarkCallbak = std::function<void(const TcpConnectionPtr&, size_t)>;