#pragma once

#include <vector>
#include <string>
#include <algorithm>

//网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;//可以记录数据包长度
    static const size_t kInitialSize = 1024;//数据包大小

    explicit Buffer(size_t initialSize = kInitialSize)
            : buffer_(kCheapPrepend + initialSize)
            , readerIndex_(kCheapPrepend)
            , writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    //onMessage Buffer 转化成 string  
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len;//应用只读取了可读缓冲区数据的一部分，就是len，还剩下readerIndex_ += len 到 writerIndex的一部分
        }
        else // len == readableBytes()
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;//复位操作，你看word文档里面的buffer图就能理解了，他就是把read那一块挤出去了
    }

    //把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); //应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);//上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    // buffer_.size() - writerIndex
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);//扩容函数
        }
    }

    //把[data，data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)//把要添加的数据添加到缓冲区中
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    //通过fd发送数据
    ssize_t writeFd(int fd, int* saveErrno);


    //httpserver测试专用
    const char* findCRLF() const;           // 查找 "\r\n"
    const char* findCRLFCRLF() const;       // 查找 "\r\n\r\n"
    void retrieveUntil(const char* end);    // 丢弃从 readerIndex_ 到 end 的数据

private:

    char* begin()
    {
        //it.operator*()
        return &*buffer_.begin();//vector底层数组首元素的地址，也就是数组的起始地址
    }


    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)//扩容函数
    {
        if (writableBytes() + prependableBytes() - kCheapPrepend < len)//这个你画个图好理解的
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readalbe = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);//拷贝begin() + readerIndex_到begin() + writerIndex_的区间，并复制到begin() + kCheapPrepend开始的位置
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_+ readalbe;
        }

    }

    std::vector<char>buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};
