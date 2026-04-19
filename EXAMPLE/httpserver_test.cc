#include <mymuduo/TcpServer.h>
#include <mymuduo/EventLoop.h>
#include <mymuduo/InetAddress.h>
#include <mymuduo/Buffer.h>
#include <mymuduo/Logger.h>
#include <string>
#include <iostream>

// 一个极简的 HTTP 服务器，专门用于 wrk 压测
class HttpServer {
public:
    HttpServer(EventLoop* loop, const InetAddress& listenAddr)
        : server_(loop, listenAddr, "HttpServer") {
        
        server_.setConnectionCallback(
            std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&HttpServer::onMessage, this, 
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        
        // 设置工作线程数，建议设置为 CPU 核心数
        server_.setThreadNum(4);
    }

    void start() {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        // 压测期间尽量少做事，不打日志
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime) {
        // 查找 HTTP 请求头结束标志 "\r\n\r\n"
        const char* end = buf->findCRLFCRLF();
        if (end) {
            // 丢弃已读的请求头数据
            buf->retrieveUntil(end + 4);

            // 固定的 HTTP 响应（务必包含 Content-Length）
            std::string body = "Hello, Muduo!";
            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: keep-alive\r\n"
                "Content-Length: " + std::to_string(body.size()) + "\r\n"
                "\r\n" +
                body;

            conn->send(response);
            // 注意：不要主动 shutdown，保持 keep-alive
        }
        // 如果请求不完整，什么都不做，等数据再来
    }

    TcpServer server_;
};

extern bool g_logEnable;

int main() {
    // 压测前务必关闭低级别日志，否则 IO 会成为瓶颈
    g_logEnable = false;   // 关闭所有非 FATAL 日志
    EventLoop loop;
    InetAddress addr(8000);               // 监听 8000 端口
    HttpServer server(&loop, addr);
    
    std::cout << "HttpServer listening on port 8000 (worker threads = 4)\n";
    server.start();
    loop.loop();
    
    return 0;
}