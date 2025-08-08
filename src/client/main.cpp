#include "json.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <vector>

using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前用户群组列表信息
vector<Group> g_currentUserGroupList;
// 控制主菜单页面程序
bool isMainMenuRunning = false;
// 显示当前登录用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int);

// 聊天客户端实现，mian线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv) // argc: 命令行参数的个数，argv: 命令行参数列表,argv[0] 是程序的名字，argv[1] 是第一个参数，argv[2] 是第二个参数，以此类推。
{
    if (argc < 3) // 检查命令行参数数量，期望的输入是 ./ChatClient 127.0.0.1 6000，这时 argc 正好是 3
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];            // argv[1]命令行输入的第一个参数, ip
    uint16_t port = atoi(argv[2]); // argv[2]命令行输入的第二个参数, port

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0); // 创建ipv4的socket
    if (clientfd == -1)                             // 创建socket失败
    {
        cerr << "create socket error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息
    sockaddr_in server;                 // 存储 IPv4 地址信息
    memset(&server, 0, sizeof(server)); // 初始化server, 将server结构体的所有字节都填充为 0

    server.sin_family = AF_INET;            // 设置为 ipv4
    server.sin_port = htons(port);          // 端口号,网络字节序(大端序)
    server.sin_addr.s_addr = inet_addr(ip); // ip地址,网络字节序(大端序)

    // client与server进行连接
    if (-1 == connect(clientfd, (struct sockaddr *)&server, sizeof(server))) // 连接失败
    {
        cerr << "connect server error" << endl;
        close(clientfd); // 连接失败, 关闭client
        exit(-1);        // 异常退出程序
    }

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页菜单：登录，注册，退出
        cout << "\n===============================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "===============================\n"
             << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车（cin的时候输入+回车，读掉cin中的回车）

        switch (choice)
        {
        case 1: // login业务
        {
            // 输入id和密码
            int id = 0;
            char pwd[50] = {0};
            cout << "userid: ";
            cin >> id; // 读掉缓冲区残留的回车
            cin.get();
            cout << "userpassword: ";
            cin.getline(pwd, 50);

            // 将用户输入封装成json发送
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump(); // 发送数据序列化

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0); // 发送数据
            if (len == -1)                                                             // 发送失败
            {
                cerr << "send login msg error" << endl;
            }
            else
            { // 发送成功，接收服务器信息并返回
                char buffer[2048] = {0};
                len = recv(clientfd, buffer, 2048, 0);
                if (-1 == len) // 接收失败
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {                                          // 接收成功
                    json responsejs = json::parse(buffer); // 接收数据反序列化
                    if (0 != responsejs["errno"])          // 登录失败
                    {
                        cerr << responsejs["errmsg"];
                    }
                    else
                    { // 登录成功
                        // 记录当前的用户信息
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"].get<string>());

                        // 记录当前好友列表
                        if (responsejs.contains("friends"))
                        {
                            // 初始化
                            g_currentUserFriendList.clear();

                            vector<string> vec = responsejs["friends"];
                            for (string &friendstr : vec)
                            {
                                json friendjs = json::parse(friendstr);
                                User myfriend;
                                myfriend.setId(friendjs["friendid"].get<int>());
                                myfriend.setName(friendjs["friendname"].get<string>());
                                myfriend.setState(friendjs["friendstate"].get<string>());
                                g_currentUserFriendList.push_back(myfriend);
                            }
                        }

                        // 记录用户的群组列表
                        if (responsejs.contains("groups"))
                        {
                            // 初始化
                            g_currentUserGroupList.clear();

                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr : vec1)
                            {
                                json groupjs = json::parse(groupstr);
                                Group group;
                                group.setId(groupjs["id"].get<int>());
                                group.setName(groupjs["groupname"].get<string>());
                                group.setDesc(groupjs["groupdesc"].get<string>());

                                vector<string> vec2 = groupjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"].get<string>());
                                    user.setState(js["state"].get<string>());
                                    user.setRole(js["role"].get<string>());
                                    group.getUsers().push_back(user);
                                }

                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        // 显示登录用户的基本信息
                        showCurrentUserData();

                        // 显示用户的离线信息（包括群组消息或者个人消息）
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                // time +  [id] + name + "said: " + msg
                                if (js["msgid"].get<int>() == ONE_CHAT_MSG) // 个人离线消息
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"] << "] "
                                         << js["name"] << " said: " << js["msg"].get<string>() << endl;
                                }
                                else // 群消息
                                {
                                    cout << "群消息[" << js["groupid"] << "] " << js["time"].get<string>()
                                         << " [" << js["id"] << "] " << js["name"] << " said: " << js["msg"].get<string>() << endl;
                                }
                            }
                        }

                        // 登录成功，启动接收线程负责接收数据,只需启动一个线程，即执行一次
                        static int num = 0;
                        if (num == 0)
                        {
                            std::thread readTask(readTaskHandler, clientfd); // 若登出，还是执行死循环，client和server未断开，clientfd还是上次创建的那个
                            readTask.detach();                               // 分离线程
                            num++;
                        }

                        // 进入聊天菜单界面
                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                }
            }
            break;
        }
        case 2: // register业务
        {
            // 输入名字和密码
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username: ";
            cin.getline(name, 50); // cin>> 读字符串时遇到空格 回车等非法字符会停止
            cout << "password: ";
            cin.getline(pwd, 50);

            // 将用户输入封装成json发送
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1) // 发送失败
            {
                cerr << "send reg info error" << endl;
            }
            else
            { // 发送成功，接收服务器信息并返回
                char buffer[2048] = {0};
                len = recv(clientfd, buffer, 2048, 0);
                if (-1 == len) // 接收失败
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                { // 接收成功
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"]) // 注册失败
                    {
                        cerr << "is already exist, register error";
                    }
                    else
                    { // 注册成功
                        cerr << "register success, userid is " << responsejs["id"]
                             << ", do not forget it!" << endl;
                    }
                }
            }
            break;
        }
        case 3: // quit业务
            close(clientfd);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
}

void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << "groupinfo" << endl;
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            cout << "group user list:" << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// 接收线程
void readTaskHandler(int clientfd)
{

    while (true)
    {
        char buffer[2048] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0); // 客户端接收数据
        if (len == -1 || len == 0)                           // 断开连接
        {
            close(clientfd);
            exit(-1);
        }

        // 客户端接收ChatService发来的数据,反序列化生成json对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "] "
                 << js["name"] << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (msgtype == GROUP_CHAT_MSG)
        {
            cout << "群消息[" << js["groupid"] << "] " << js["time"].get<string>()
                 << " [" << js["id"] << "] " << js["name"] << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat:groupid:message"},
    {"loginout", "登出,格式loginout"}};

// 注册系统支持的客户端命令处理函数
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[2048] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 2048);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":"); // 查询有:的字符串
        if (idx == -1)                  // 未找到带:的字符串
        {
            command = commandbuf; // help,loginout或其他未定义的不带:的命令
        }
        else // 找到带:的字符串
        {
            command = commandbuf.substr(0, idx); // 获取第一个:前的字符串，即命令
        }
        // 处理命令
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        { // 不在在命令表中
            cerr << "invalid input command!" << endl;
            continue;
        }
        // 找到命令,并调用相对应的事件处理回调
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 将第一个:后的字符串作为参数传递给回调函数
    }
}

// "help" command handler
void help(int clientfd, string str)
{
    cout << "Supported commands>>> " << endl;
    for (auto &command : commandMap)
    {
        cout << command.first << ": " << command.second << endl;
    }
    cout << endl;
}

// "chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "chat command invalid!" << endl; // 聊天参数有两个，用:分开，若没有: ，表明参数错误
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error ->" << buffer << endl;
    }
}

// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error ->" << buffer << endl;
    }
}

// "creategroup" command handler
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error ->" << buffer << endl;
    }
}

// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = JOIN_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error ->" << buffer << endl;
    }
}

// "groupchat" command handler
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cout << "groupchat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error ->" << buffer << endl;
    }
}

// "loginout" command handler
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg error ->" << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}