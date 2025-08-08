/*
muduo网络库给用户提供了两个主要的类：
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

epoll + 线程池
好处：将网络IO的代码和业务代码区分开
                    用户的连接与断开  用户的可读写事件
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;
/*基于网络moduo库开发服务器程序
    1.组合TcpServer对象
    2.创建EventLoop事件循环对象的指针
    3.明确TcpServer的构造函数的参数，输出ChatServer的构造函数
    4.在当前服务器类的构造函数中，注册用户连接断开的回调，用户的读写事件的回调
    5.设置合适的服务端线程数量（muduo库会自己分配I/O线程和worker线程）
*/

class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP+端口
               const string &nameArg)         // 服务器名
        : _server(loop, listenAddr, nameArg), _loop(loop)

    {
        // 给服务器注册用户连接的创建和断开回调
        // setConnectionCallback接受一个const TcpConnectionPtr&参数，onConnection需要两个参数：一个 ChatServer* (即 this) 和一个 const TcpConnectionPtr&
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // this是调用onConnection的对象
                                                                                       //_1是占位符，表示函数的第一个参数，调用时补上
        // 给服务器注册用户的读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量，1个IO线程,3个工作线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门处理用户连接和断开   epoll listened accept
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " is online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << " -> "
                 << conn->localAddress().toIpPort() << " is offline" << endl;
            conn->shutdown(); // close(fd)
            //_loop->quit(); // 停止事件循环
        }
    }
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 指向缓冲区
                   Timestamp time)               // 接收数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv:" << buf << " time：" << time.toString() << endl;  // 接收
        conn->send(buf);  // 回显
    }
    TcpServer _server;
    EventLoop *_loop; // epoll
};

int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();  // epoll.wait以阻塞的方式等待新用户连接，已连接用户的读写事件等
    return 0;
}