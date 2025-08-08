#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include <functional>
#include <string>

using namespace std;
using json = nlohmann::json;
using namespace placeholders;
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
     
    // 注册链接回调, 在新连接建立或断开时，自动地、异步地来调用onConnection
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1)); // 非静态成员函数,函数名不是指针，需要&

    // 注册消息回调
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量, 1个IO线程，3个worker线程
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if (!conn->connected())
    {
        //处理服务器的异常退出：将数据库state更新为offline
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);
    // 目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"]获取 =》业务handler =》conn js time 传入参数
    auto handler = ChatService::instance()->getHandler(js["msgid"].get<int>()); // get方法将json值强转为int类型
    // 通过回调已经绑定好的事件处理器，执行对应的业务处理
    handler(conn, js, time);
}