#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis
{ 
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的channel发布消息
    bool publish(int channel, string message);

    // 向redis指定的channel订阅消息
    bool subscribe(int channel);

    // 向redis指定的channel取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接受订阅通道中的消息
    void observe_channel_message();

    // 初始化向业务层上报消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);

private:
    // hiredis同步上下文对象，负责subcribe消息  （由于redis中执行subcribe时会阻塞程序，所以要分出两个连接对象处理）
    redisContext *_subscribe_context;

    // hiredis同步上下文对象，负责publish消息    （publish不会阻塞）
    redisContext *_publish_context;

    // 回调函数，收到订阅的消息，给service层上报
    function<void(int, string)> _notify_message_handler;  // int 指示channel，string 指示消息
};

#endif