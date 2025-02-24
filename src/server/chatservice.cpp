#include "chatservice.hpp"
#include "public.hpp"
#include "group.hpp"

#include <muduo/base/Logging.h>
#include <vector>

// 数据量说有个1万几就好

using namespace std;
using namespace muduo;

// 处理业务操作

// 用户注册测试语句：{"msgid":3, "name":"wuyaofeng", "password":"123456"}
// 用户登录语句：
// 登录1：{"msgid":1, "id":24, "password":"123456"}
// 登录2：{"msgid":1, "id":25, "password":"123456"}
// 聊天消息 wuyaofeng 对 sakura：{"msgid":5,"id":24,"from":"wuyaofeng", "to": 25, "msg": "sakura Hello 2222"} // 有中文，或则给表上锁了，插入就会失败
// 聊天消息 sakura 对 wuyaofeng：{"msgid":5,"id":25,"from":"sakura", "to": 24, "msg": "wuyaofeng Hello"}



// 好友测试：
// 添加好友：{"msgid":6,"id":24, "friendid": 25}


// 获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std :: bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std :: bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std :: bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std :: bind(&ChatService :: addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 处理登录业务 id password
// ORM 业务层操作的都是对象 DAO 数据层操作的是数据库
void ChatService :: login (const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id); // 通过主键查找到user

    // 判断查找到的id是否真的等于输入的id
    // 判断数据库存储的密码 和 用户输入的密码是否相同
    if (user.getId() == id && user.getPassword() == pwd)
    {
        if (user.getState() == "online")
        {
            // 查询均正确，但该用户已登录，不允许重复登录
            json response; 
            response["msgid"] = LOGIN_MSG_ACK;
            response["error"] = 2; 
            response["errmsg"] = "this account is using, input another";
            conn->send(response.dump()); 


            // 【扩展】该账号 已经被登录，是否在该设备重新登录
        }
        else
        {
            // 登录成功，记录用户连接信息 【涉及了多个用户（即多个线程的访问）】需要考虑线程安全
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({user.getId(), conn});
            }

            // id 用户登录后，向redis订阅channel(id)
            _redis.subscribe(user.getId());




            // 登录成功，更新用户状态信息 state offline => online 
            user.setState("online");
            _userModel.updateState(user);



            json response; 
            response["msgid"] = LOGIN_MSG_ACK;
            response["error"] = 0; // 表示成功，即无错
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);

            // 查询是否具有离线消息
            if (!vec.empty())
            {
                response["offlinemsg"] = vec; // 将离线消息传回

                // 读取该用户的离线消息以后，把该用户的所有离线消息进行删除
                // 保证用户，再次登录不会重复接受相同的离线消息
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友消息并返回
            vector<User> userVec = _friendModel.query(id);
            // 该用户确实有好友
            if (!userVec.empty())
            {
                vector<string> vecStr;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();

                    vecStr.push_back(js.dump());
                }

                response["friends"] = vecStr;
            }

            // 查询该用户是否有加入群
            vector<Group> userGroup = _groupModel.queryGroups(id);
            if (!userGroup.empty())
            {
                vector<string> vecStr;
                for (Group &group : userGroup)
                {
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();
                    

                    // 拿用户
                    vector<string> vecStr2;
                    for (GroupUser &groupuser : group.getUsers())
                    {
                        json js2;
                        js2["id"] = groupuser.getId();
                        js2["name"] = groupuser.getName();
                        js2["state"] = groupuser.getState();
                        js2["role"] = groupuser.getRole();

                        vecStr2.push_back(js2.dump());
                    }

                    js["users"] = vecStr2;

                    vecStr.push_back(js.dump());
                }

                response["groups"] = vecStr;
            }

            conn->send(response.dump()); // 将登录成功的消息发送回给客户端
        }
        



    }
    else
    {
        // 该用户不存在 或用户存在，但密码错误， 登录失败
        json response; 
        response["msgid"] = LOGIN_MSG_ACK;
        response["error"] = 1; 
        response["errmsg"] = "id or password is invalid";
        conn->send(response.dump()); 
    }
}

// 处理注册业务 name password
void ChatService :: reg (const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    // 将新数据，映射为类
    User user;
    user.setName(name);
    user.setPassword(pwd);
    

    // 插入新用户
    bool insertState = _userModel.insert(user);
    if (insertState)
    {
        // 注册成功
        json response; 
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 0; // 表示成功，即无错
        response["id"] = user.getId();
        conn->send(response.dump()); // 将注册成功的消息发送回给客户端
    }
    else
    {
        // 注册失败
        json response; 
        response["msgid"] = REG_MSG_ACK;
        response["error"] = 1; // 表示有错误，即注册失败;

        conn->send(response.dump()); // 将注册失败的消息发送回给客户端
    }
}

// 一对一聊天业务 记得用json表做好数据的约束
void ChatService :: oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>(); // 信息接受方的id

    {
        // 访问连接信息表，保证线程安全 避免connection在发送消息时，被其他程序移除
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);

        if (it != _userConnMap.end())
        {
            // 说明 toid 在线 转发消息 服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询 toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        // 说明在别的服务器上 通过redis传递消息
        _redis.publish(toid, js.dump());
        return;
    }


    // told不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}


// 添加好友 业务
// 会接收到一个json表，这个json表能够用于添加当前【conn用户】的好友，并存储到friend表中
void ChatService :: addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 当前用户的id
    int friendid = js["friendid"].get<int>(); // 当前用户，好友的id

    // 存储好友信息
    _friendModel.insert(userid, friendid);

    _friendModel.insert(friendid, userid);
}

// 创建群组 业务
void ChatService :: createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 创建群的用户 id
    string name = js["groupname"]; // 创建群的名字 groupname
    string desc = js["groupdesc"]; // 创建群的描述 groupDesc

    // 存储新创建的群组信息
    Group group(-1, name, desc);

    // 避免重复创建
    if (_groupModel.createGroup(group))
    {
        // 创建成功，存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组 业务
void ChatService :: addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务 【核心逻辑：发送的话，除了本人外，其他人都能接收到】
void ChatService :: groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 说话人的id
    int groupid = js["groupid"].get<int>(); // 群主 id
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 用户处于连接状态
            // 转发群聊天消息
            it -> second -> send(js.dump());
        }
        else
        {
            // 查询 toid是否在线，在线说明在别的服务器
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                // 通过redis将消息发送给对方
                _redis.publish(id, js.dump());
            }
            else
            {
                // 真的不在线，存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }


            
        }
    }
}


// 获取消息对应的处理器
MsgHandler ChatService :: getHandler(int msgid)
{
    // 记录错误日志，即msgid，没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        
        // 返回一个默认的处理器， 空操作
        return [=](const TcpConnectionPtr& conn, json &js, Timestamp){
            LOG_ERROR << "msgid: " << msgid << "can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }

    
}

// 服务器异常，业务重置方法
void ChatService :: reset()
{
    // 把 online 状态的用户，设置成 offline 重置用户状态信息
    _userModel.resetState();
}

// 处理客户端异常退出
// 在终端，采用按空格的方式，不会进行更新
void ChatService ::  clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    // 删除 该连接对应的 _userConnMap的数据
    // 用户有可能处于 正在登陆状态，即往 _userConnMap中写入数据
    // 因此要开锁
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it=_userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            // 找到异常退出的连接
            if (it -> second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(it -> first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }

}



// 处理 注销业务
void ChatService :: loginout(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);

        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户状态信息
    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updateState(user);
}




// 从redis消息队列中获取订阅的消息
void ChatService :: handleRedisSubscribeMessage(int userid, string msg)
{
    json js = json :: parse(msg.c_str());

    lock_guard<mutex> lock(_connMutex);

    // 该用户可能是，在对方发送完消息后下线的
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it -> second -> send(js.dump());
        return;
    }

    // 存储用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}

