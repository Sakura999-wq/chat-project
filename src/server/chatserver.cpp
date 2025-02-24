#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
using namespace std;
using namespace placeholders;
using json = nlohmann :: json;

ChatServer :: ChatServer (EventLoop* loop, 
                    const InetAddress& listenAddr, 
                    const string& nameArg)
                    : _server(loop, listenAddr, nameArg)
                    , _loop(loop)
{
    // 注册连接的回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息 回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer :: start()
{
    _server.start();
}


// 上报连接相关信息的 回调函数
void ChatServer :: onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if (!conn -> connected())
    {
        // 根据异常退出，更新用户的信息
        ChatService :: instance() -> clientCloseException(conn);
        conn -> shutdown();
    }
}

// 上报读写事件相关信息的 回调函数
void ChatServer :: onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    string buf = buffer -> retrieveAllAsString(); // 将缓存区的数据，放到字符串中
    
    // 数据的反序列化
    json js = json :: parse(buf); // message type message id
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过 js["msgid"] 获取 一个 业务Handler【网络模块看不见，在业务模块进行编辑的】解耦
    // 避免 if （msgid） 做什么事，这种情况

    // 获取 msgid 对应的消息处理器
    auto msghandler = ChatService::instance()->getHandler(js["msgid"].get<int>());

    // 让消息处理器，去执行其对应的业务操作
    msghandler(conn, js, time);
    
}

