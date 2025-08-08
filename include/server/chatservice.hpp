#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "json.hpp"
#include "redis.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 聊天服务器业务类（单例设计模式）
class ChatService
{

public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理登出业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 获取事件处理器
    MsgHandler getHandler(int msgid);
    // 处理用户端的异常退出(客户端通过ctrl+] 后 quit主动退出，重置客户端状态信息)
    void clientCloseException(const TcpConnectionPtr &conn);
    // 服务器异常，业务重置方法（服务器通过ctrl + c断开，重置客户端状态信息）
    void reset();

    // 从redis消息队列中获取消息
    void handlerRedisSubscribeMessage(int, string);

private:
    ChatService(); // 构造函数私有化
    // 存储消息id和对应业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap; 
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;  // map不是线程安全的，对其进行增删改查时要加锁
    // 互斥锁，保护_userConnMap
    mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // redis操作类对象
    Redis _redis;
};

#endif