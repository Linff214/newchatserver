#include "redis.hpp"
#include <iostream>
using namespace std;
#include "json.hpp"
using json = nlohmann::json;
Redis::Redis()
    : _publish_context(nullptr), _subcribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subcribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
// 重载 publish 方法，支持字符串频道
bool Redis::publish(std::string channel, std::string message)
{
    redisReply* reply = (redisReply*)redisCommand(_publish_context, "PUBLISH %s %s", channel.c_str(), message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}
bool Redis::subscribe(const string &channel)
{
    // SUBSCRIBE 命令：订阅指定字符串频道
    // 这里只做订阅，不接收消息，接收消息在 observer_channel_message() 中处理
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %s", channel.c_str()))
    {
        cerr << "subscribe to channel [" << channel << "] failed!" << endl;
        return false;
    }

    // redisBufferWrite 可以循环发送缓冲区，直到缓冲区数据发送完毕
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "subscribe to channel [" << channel << "] failed during buffer write!" << endl;
            return false;
        }
    }

    // 这里不调用 redisGetReply()，因为真正的消息处理在 `observer_channel_message()` 中进行
    cout << "Subscribed to channel [" << channel << "] successfully!" << endl;
    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subcribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3 &&
            reply->element[2] != nullptr && reply->element[2]->str != nullptr)
            {
                cout << ">>> Received message from channel [" << reply->element[1]->str << "]: " 
                     << reply->element[2]->str << endl;
    
                // 尝试解析 JSON 并调用回调
                try
                {
                    string channel(reply->element[1]->str);
                    string message(reply->element[2]->str);
    
                    // 检查频道是否是 system_broadcast
                    if (channel == "system_broadcast")
                    {
                        cout << ">>> Forwarding broadcast message to handler..." << endl;
                        _notify_message_handler(0, message);  // 使用 0 作为 system_broadcast 的标识
                    }
                    else
                    {
                        // 处理其他 channel 时转换为 int
                        int userid = atoi(channel.c_str());
                        cout << ">>> Forwarding message to user [" << userid << "]" << endl;
                        _notify_message_handler(userid, message);
                    }

                }
                catch (const exception &e)
                {
                    cerr << "Error parsing message: " << e.what() << endl;
                }
            }
            freeReplyObject(reply);
        }
    
        cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
    }


void Redis::init_notify_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler = fn;
}