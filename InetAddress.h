#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>//sockaddr_in
#include <string>

//封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {}

    std::string toIp() const;//获取ip
    std::string toIpPort() const;//获取ip port
    uint16_t toPort() const;//获取端口号

    const sockaddr_in* getSockAddr() const {return &addr_;}//获取成员变量
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr;}
private:
    sockaddr_in addr_;
};