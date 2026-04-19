#include "Buffer.h"

#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>//write
#include <algorithm>      // std::search
#include <cassert>        // assert

//http测试专用
static const char kCRLF[] = "\r\n";
static const char kCRLFCRLF[] = "\r\n\r\n";

/*
* 从fd上读取数据Poller工作在LT模式
* Buffer缓冲区是有大小的！但是从fd上读数据的时候，却不知道tcp数据最终的大小
*/
ssize_t Buffer::readFd(int fd,int* saveErrno)
{
    char extrabuf[65536]={0};//栈上的内存空间 64KB

    struct iovec vec[2];

    const size_t writable = writableBytes();//这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;//buffer可写空间小于64KB，就传2个；大于64KB，就传1个

    const ssize_t n = ::readv(fd, vec, iovcnt);//读取的字节数

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)//Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else//extrabuf里面也写入了数据
    {
        writerIndex_ =buffer_.size();
        append(extrabuf, n - writable);//writerIndex_开始写n-writable大小的数据
    }

    return n;
}

ssize_t Buffer::writeFd(int fd,int* saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}


const char* Buffer::findCRLF() const {
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
}

const char* Buffer::findCRLFCRLF() const {
    const char* crlfcrlf = std::search(peek(), beginWrite(), kCRLFCRLF, kCRLFCRLF + 4);
    return crlfcrlf == beginWrite() ? nullptr : crlfcrlf;
}

void Buffer::retrieveUntil(const char* end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
}