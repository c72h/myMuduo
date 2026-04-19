#pragma once

#include "noncopyable.h"

class InetAddress;

//封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
        :sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress &localaddr);
    void listen();//监听
    int accept(InetAddress *peeraddr);//从监听队里面拿通信的fd

    void shutdownWrite();//tcp是一个双端通信，每一个段都可读可写，这个是关闭写

    void setTcpNoDelay(bool on);//直接发送，不进行数据缓冲
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};
