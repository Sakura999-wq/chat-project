// 业务操作

#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex> // 用来加锁

#include "redis.hpp"
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 消息id，对应的回调
// 表示处理消息的事件回调方法类型
using MsgHandler = std :: function<void(const TcpConnectionPtr& conn, json &js, Timestamp)>;

// 聊天服务器业务类 单例类
// 一个消息id，映射一个业务处理【其实跟C#对怪物死亡这种处理有点像】，目的就是解耦
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();

    // 处理登录业务
    void login (const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg (const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友 业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组 业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组 业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天 业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理 注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 服务器异常，业务重置方法
    void reset();

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);


    // 处理 redis回调操作
    void handleRedisSubscribeMessage(int, string);

private:
    // 构造函数私有化
    ChatService();

    
    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap 的线程安全
    mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // redis操作对象
    Redis _redis;
};


#endif // !CHATSERVICE_H

