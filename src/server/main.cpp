#include "chatserver.hpp"
#include "signal.h"
#include "chatservice.hpp"

#include <iostream>

void resetHandler(int)    // int signum参数是触发信号处理器的信号编号,这里只处理一种信号，所以不需要使用
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3) // 检查命令行参数数量，期望的输入是 ./ChatClient 127.0.0.1 6000，这时 argc 正好是 3
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    
    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];            // argv[1]命令行输入的第一个参数, ip
    uint16_t port = atoi(argv[2]); // argv[2]命令行输入的第二个参数, port

    signal(SIGINT, resetHandler); // SIGINT表示键盘输入ctrl+c

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer chatserver(&loop, addr, "ChatServer");
    chatserver.start();

    loop.loop(); // 启动事件循环，等待事件 处理活跃事件 分发回调

    return 0;
}