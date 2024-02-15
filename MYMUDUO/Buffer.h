#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include <algorithm>


/**
 * ------------------------------------------------------------ 
 * | prependable Bytes | readable Bytes | writable Bytes      | 
 * |  可预留字节        |  可读取字节     | 可写入字节           |
 * |  可以存储消息头部   | 可被程序读取的  | 缓冲区未被使用的空间  |
 * ------------------------------------------------------------
 */
//网络库底层的缓冲区类型
class Buffer
{
public:
    static const size_t kCheapPrepend = 0;
    static const size_t kInitialSize = 0;


    explicit Buffer(size_t initialSize = kInitialSize)
        :buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
    {}

    //缓冲区可读字节数
    size_t readableBytes() const 
    {
        /**
         * 可写位置-可读的位置，为可读字节数
         * 
         *    |可读位置|    |可写位置|
         *        ↑            ↑
         *    |头||<-可读空间->||<-可写空间->|
         *    <---------缓冲区空间---------->    
         */
        return writerIndex_ - readerIndex_;
    }

    //获取缓冲区可写的字节数
    size_t writeableBytes() const 
    {
        return buffer_.size() - writerIndex_;
    }

    //获取缓冲区预留字节数，头部,包含可读缓冲区已经读出来的字节数
    size_t prependableBytes() const 
    {
        return readerIndex_;
    }

    //获取缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    //修改缓冲区的情况
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            //只读了len，所以可读位置后移len
            readerIndex_ += len;
        }
        else //len == readableBytes()
        {
            retrieveAll();
        }
    }

    //缓冲区数据被读完
    void retrieveAll()
    {
        //读写位置都要归零，因为没有数据了
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    //把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        //上面获取了缓冲区的数据，所以要对缓冲区进行调整
        retrieve(len);
        return result;
    }

    //保证缓冲区大小足够保存准备写入的数据
    void ensureWriteableBytes(size_t len)
    {
        if(writeableBytes() < len)
        {
            //扩容函数
            makeSpace(len);
        }
    }

    //向可写空间追加数据
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        //可写空间添加了len数据，可写位置后移len
        writerIndex_ += len;
    }

    //得到可写位置的地址
    char* beginWrite()
    {
        return begin() + writerIndex_;
    }
    const char* beginWrite() const 
    {
        return begin() + writerIndex_;
    }

    //从fd中读取数据
    ssize_t readFd(int fd, int *saveErrno);
    //向fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char* begin()
    {
        return &*buffer_.begin();  //数组起始地址
    }
    const char* begin() const 
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        if(writeableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            //可读缓冲区大小
            size_t readable = readableBytes();

            /**
             * copy(a, b, c);
             * 将a到b之间的数据，写到c位置的后面 
             */
            std::copy(begin() + readerIndex_, 
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    //存放发送的数据
    std::vector<char> buffer_;
    //可读数据的位置
    size_t readerIndex_;
    //可写数据的位置
    size_t writerIndex_;

};
