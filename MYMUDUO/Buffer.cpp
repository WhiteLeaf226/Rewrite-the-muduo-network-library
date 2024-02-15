#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include "Buffer.h"

ssize_t Buffer::readFd(int fd , int *saveErrno)
{
    //64k空间
    char extrabuf[65536];

    //两个不连续缓冲区  
    struct iovec vec[2];

    //可写空间大小
    const size_t writable = writeableBytes();

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    //选择一个缓冲区还是两个
    const int iovcnt = writable < sizeof(extrabuf) ? 2 : 1;
    
    const ssize_t n = ::readv(fd, vec, iovcnt);
    
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writable) //buf可写缓冲区足够存放数据
    {
        writerIndex_ += n;
    }
    else //n > writable buf缓冲区不够，数据存在了两个缓冲区
    {
        //改变可写位置，将extrabuf中数据写入缓冲区
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *saveErrno = errno;
    }

    return n;
}