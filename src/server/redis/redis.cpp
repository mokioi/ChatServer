#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis() : _subscribe_context(nullptr), _publish_context(nullptr)
{
}

Redis::~Redis()
{
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }

    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文消息
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文信息
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (_subscribe_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息时向业务层上报
    thread t([&]()
             { observe_channel_message(); });
    t.detach();

    cout << "connect redis-server success!";
    return true;
}

// 向redis指定的channel发布消息
bool Redis::publish(int channel, string message)
{
    // 通过redisCommand发送redis指令并执行
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!";
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的channel订阅消息
bool Redis::subscribe(int channel)
{
    // subscribe命令会阻塞程序，这里仅订阅通道，不接收通道消息，防止和notifyMsg线程抢占响应资源
    // subscribe = redisAppendCommand + redisBufferWrite + redisGetReply
    // redisAppendCommand 将命令写到本地缓存，redisBufferWrite将本地缓存发送给redis服务器，redisGetReply以阻塞方式等待远端响应
    // 将接收消息放在observe_channel_message()的独立线程中执行
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!";
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!";
            return false;
        }
    }

    // redisGetReply
    return true;
}

// 向redis指定的channel取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!";
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!";
            return false;
        }
    }

    // redisGetReply
    return true;
}

// 在独立线程中接受订阅通道中的消息
void Redis::observe_channel_message()
{
    redisReply *reply = NULL;
    // 以阻塞的方式接受消息
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        // 订阅收到的消息是带三个元素的数组，element[0]是类型，element[1]是通道，element[2]是订阅的消息
        if (reply != NULL && reply->element[2] != NULL && reply->element[2]->str != NULL)
        {
            // 给业务层上报消息(调用回调函数)
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>>>>>>>observe_channel_message quit<<<<<<<<<<<<<<<<<<";
}

// 初始化向业务层上报消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}