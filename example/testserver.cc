#include "../TcpServer.h"
#include "../Logger.h"

#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name), loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置合适的loop线程数量 loopthread
        server_.setThreadNum(3);
    }

    /**
     * TcpServer.start意味着开启处于mainLoop中的成员变量Acceptor的listen监听
     */
    void start()
    {
        server_.start();
    }

private:
    // 连接建立或者断开的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); // 写端   EPOLLHUP =》 closeCallback_
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    // Acceptor non-blocking listenfd  create bind
    // 对于TcpServer的初始化操作，只有进行了初始化才能继续接下来的start操作
    EchoServer server(&loop, addr, "EchoServer-01");
    // listen  loopthread  listenfd => acceptChannel => mainLoop =>
    server.start();
    /**
     * 通过loop.loop()启动mainLoop的底层Poller，这是main线程的Loop
     * subLoop的loop函数将在EventLoopThread中定义的线程函数threadFunc中开启，
     * 即启动一个线程就开启一个EventLoop
     */
    loop.loop();

    return 0;
}