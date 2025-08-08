#include "chatservice.hpp"
#include "public.hpp"
#include "muduo/base/Logging.h"
#include <vector>

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{ // 静态方法在类外实现不用写static关键字
    static ChatService service;
    return &service;
}

// 注册消息以及对应Handler回调操作
ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({JOIN_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调, 将handlerRedisSubscribeMessage回调函数初始化为_notify_message_handler
        _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMessage, this, _1, _2));
    }
}

// 获取事件处理器
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) { // 遵守类型系统的“接口统一”原则，完全相同的调用接口（参数列表和返回值类型）
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 登录业务， 需要id和password
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    if (user.getId() != -1 && user.getPwd() == pwd) // 默认用户id为-1
    {
        if (user.getState() == "online")
        {
            // 用户已经登录,不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, please use another one!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn}); // 添加用户连接信息（该操作需要加锁）
            }

            // 登录成功，向redis服务器订阅channel
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息, setState: offline -> online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询该用户的离线消息
            vector<string> offlineMsg = _offlineMsgModel.query(id);
            if (!offlineMsg.empty())
            {
                response["offlinemsg"] = offlineMsg;
                // 读取该用户的所有离线消息并清空
                _offlineMsgModel.remove(id);
            }
            // 显示好友信息
            vector<User> userVec = _friendModel.query(id); // 获取用户所有好友User对象
            if (!userVec.empty())                          // 存在好友
            {
                vector<string> ver2; // 存储所有好友信息
                for (User &myfriend : userVec)
                {
                    json js;
                    js["friendid"] = myfriend.getId();
                    js["friendname"] = myfriend.getName();
                    js["friendstate"] = myfriend.getState();
                    ver2.push_back(js.dump());
                }
                response["friends"] = ver2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 登录失败 （1. 用户不存在：id == -1     2. 用户存在但密码错误）
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 注册业务， 需要name和password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user); // insert内部进行了数据库连接操作以及插入操作
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;         // 错误编码，0表示无错误
        response["id"] = user.getId(); // 注册成功后，就可以获取到用户id
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1; // 错误编码，1表示发生错误
        conn->send(response.dump());
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();     // 获取接收用户id
    User user = _userModel.query(toid); // 获取接收客户端user
    if (user.getId() != -1)             // 账户存在
    {
        if (user.getState() == "online") // 查询账户是否在线
        {
            lock_guard<mutex> lock(_connMutex);
            auto it = _userConnMap.find(toid);
            if (it != _userConnMap.end())
            {
                // toid用户在线，且在本机找到，向接收客户端发送消息
                it->second->send(js.dump());
                return;
            }
            else // 跨服聊天：用户在线，但本机找不到
            {
                _redis.publish(toid, js.dump());
            }
        }
        // toid用户不在线，存储离线消息
        _offlineMsgModel.insert(toid, js.dump());
    }
}

// 处理登出业务
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 从map表删除用户的连接信息(该操作需要加锁)
            _userConnMap.erase(it);
        }
    }

    // 用户登出，向redis取消订阅channel
    _redis.unsubscribe(id);

    // 更新用户状态信息
    User user(id, "", "", "offline");
    _userModel.updateState(user);
}

// 处理用户的异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息(该操作需要加锁)
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户异常退出，向redis服务器中取消订阅channel
    _redis.unsubscribe(user.getId());

    // 更新用户状态信息
    if (user.getId() != -1) // 账户存在
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 服务器异常，业务重置方法（服务器通过ctrl + c断开，重置客户端状态信息）
void ChatService::reset()
{
    _userModel.resetState();
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    // 存储新创建的群组信息
    if (_groupModel.createGroup(group)) // 创建群组成功
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id); // 在线用户中查找
        if (it != _userConnMap.end())
        {
            // 本机找到，且用户在线，转发群信息
            it->second->send(js.dump());
        }
        else // 两种情况：1.本机找不到，但用户在线  2. 本机找不到，用户不在线
        {
            User user = _userModel.query(id); // 查询数据库
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump()); // 跨服聊天
            }
            else // 用户离线,存储离线群信息
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}


// 从redis消息队列中获取订阅的消息
void ChatService::handlerRedisSubscribeMessage(int channel, string message)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(channel);
    if (it != _userConnMap.end()) // 用户在线
    {
        it->second->send(message);
        return; 
    }
    // 存储该用户的离线消息
    _offlineMsgModel.insert(channel, message);  
}