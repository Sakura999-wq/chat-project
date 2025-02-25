#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace std;
using json = nlohmann :: json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

/* 
    1.没有考虑高并发
    2. 服务器和客户端 通过json进行交互
*/

// 记录当前系统登录的用户信息
User g_currentUser;

// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;

// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;

// 记录登录状态
atomic_bool g_isLoginSuccess{false};

// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间 （聊天信息需要添加时间信息）
string getCurrentTime();

// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现 main线程用作发送线程 子线程用作接收线程
// TCP socket 套接字 通信机制
// 主线程专门做发送线程，子线程专门做接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid ! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和poot
    char *ip = argv[1]; // 拿ip地址
    uint16_t port = atoi(argv[2]); // 拿端口号

    // 创建 client端的socket TCP
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写 client需要连接的server信息 ip + port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET; // 指定IP地址版本为IPV4
    server.sin_port = htons(port); // 写入端口号
    server.sin_addr.s_addr = inet_addr(ip); // 写入ip地址

    // client 和 server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr <<  "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功 启动接收子线程【专门做接收操作】
    std :: thread readTask(readTaskHandler, clientfd); // 底层调用 linux的 pthread_create
    readTask.detach(); // 底层调用 linux的 pthread_detach 线程用完后自动回收

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "============================" << endl;
        cout << "1. login" << endl;
        cout << "2. regisetr" << endl;
        cout << "3. quit" << endl;
        cout << "=============================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车


        switch (choice)
        {
            case 1: // login 业务
            {
                int id = 0;
                char pwd[50] = {0};
                cout << "userid: ";
                cin >> id;
                cin.get(); // 读取掉缓冲区残留的回车
                cout << "user password: ";
                cin.getline(pwd, 50);

                // 编写 发送给服务端的json
                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();
                
                g_isLoginSuccess = false;

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "send login msg error: " << request << endl;
                }

                // 阻塞等待 信号量，由子线程【readThread】处理完登录的相应消息后，通知这里
                sem_wait(&rwsem);

                if (g_isLoginSuccess)
                {
                    // 登录成功
                    isMainMenuRunning = true;
                    mainMenu(clientfd);
                }


            }
            break;
            
            case 2: // regiseter 业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username: ";
                cin.getline(name, 50);
                cout << "password: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump(); // 将json，转为字符串

                // 客户端将请求发送给服务端
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                // 约定：收到 -1 就是 数据发送失败
                if (len == -1)
                {
                    cerr << "send reg msg error: " << request << endl;
                }

                sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会进行通知

                break;
            }

            
            case 3: // quit 业务
            {
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
                break;
            }

            default: // 无效业务
            {
                cerr << "invalid input !" << endl;
                break;
            }

        }
    }
}


// 显示当前登录用户的基本信息
void showCurrentUserData()
{
    cout << "================== login user ======================"<< endl;
    cout << "current login user => id: " << g_currentUser.getId() << "  name: " << g_currentUser.getName() << endl;
    cout << "--------------------------- friend list ------------------------" << endl;

    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }

    cout << "-------------------- group list --------------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers()) // 群成员列表
            {
                // 群成员更具体的信息
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "====================================" << endl;
}


// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    // 客户端将服务端发送回来的数据，进行反序列化

    if (0 != responsejs["error"].get<int>())
    {
        // 说明登录出错
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else
    {
        // 登录成功

        // 设置用户信息
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);


        // 看用户是否包含好友信息
        if (responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();
            

            // 包含，拿到好友
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json :: parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();


            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json grpjs = json :: parse(groupstr); // 反序列化
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                // 群组内的用户
                vector<string> vec2 = grpjs["users"];
                for (string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json :: parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                
                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息 个人聊天消息或群组消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json :: parse(str);

                int msgtype = js["msgid"].get<int>();
                if (ONE_CHAT_MSG == msgtype)
                {
                    //打印接收到的消息： time + [id] + name + said: + msg
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                        << "said: " << js["msg"].get<string>() << endl;
        

                }
                else if (GROUP_CHAT_MSG == msgtype) // 群聊
                {
                    //打印接收到的消息：群消息[groupid]： time + [id] + name + said: + msg
                    cout << "群消息[ " << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                        << "said: " << js["msg"].get<string>() << endl;
        
                }
            }
        }

        g_isLoginSuccess = true;
    }
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{

    // 服务端成功发回，关于注册操作的一些响应
    if (0 != responsejs["error"].get<int>()) // 自己编写的服务端，如果不为0，说明注册出错
    {
        // 数据库插入的时候，没插进去，说明名字已经存在
        cerr << "is already exist, register error !" << endl;
    }
    else // 注册成功
    {
        // 注册成功，并将用户号返回
        cout <<"register success, userid is " << responsejs["id"] << ", do not forget it !" << endl;
    }
}


// 子线程：接收线程

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 接收服务器发送回来的消息 阻塞进程

        // 接收异常，或是接收到的数据为空，都关闭这个线程
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据
        // 反序列化接收到的 json数据
        json js = json :: parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            //打印接收到的消息： time + [id] + name + said: + msg
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                << "said: " << js["msg"].get<string>() << endl;

            continue;
        }

        if (GROUP_CHAT_MSG == msgtype) // 群聊
        {
            //打印接收到的消息：群消息[groupid]： time + [id] + name + said: + msg
            cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                << "said: " << js["msg"].get<string>() << endl;

            continue;
        }

        if (LOGIN_MSG_ACK == msgtype) // 登录应答
        {
            // 接收到登录响应时，进入doLoginResponse处理登录的业务逻辑
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem); // 通知主线程，登录结果处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem); // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// 获取系统时间 （聊天信息需要添加时间信息）
string getCurrentTime()
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    
    // 将时间点转换为 time_t 类型
    auto tt = std::chrono::system_clock::to_time_t(now);
    
    // 将 time_t 转换为本地时间
    std::tm tm = *std::localtime(&tt);
    
    // 使用 stringstream 格式化时间
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S"); // 格式化为 "年-月-日 时:分:秒"
    
    // 返回格式化后的时间字符串
    return ss.str();
}

void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);





// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = 
{
    {"help", "显示所有支持的命令, 格式help"},
    {"chat", "一对一聊天, 格式 chat:friend:message"},
    {"addfriend", "添加好友, 格式 addfriend:friendid"},
    {"creategroup", "创建群组, 格式 creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组, 格式 addgroup:groupid"},
    {"groupchat", "群聊, 格式 groupchat:groupid:message"},
    {"loginout", "注销, 格式 loginout"}
};



// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHanlerMap = 
{
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}

};



// 主聊天 页面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":"); 
        if (-1 == idx)
        {
            // 找到第一个带：的，就是用户所输入的命令
            command = commandbuf; 
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHanlerMap.find(command); // 发现所输入的命令，对应的执行函数

        // cout << "输入的命令为：" << command << endl;
        // 表示找不到 -- 即用户输入的命令是错误的
        if (it == commandHanlerMap.end())
        {
            cerr << "invalid input command !" << endl;
            continue;
        }

        // 调用相应的命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it -> second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() -idx)); // 调用命令所执行的方法
    }
}



void help(int clientfd, string)
{
    // 显示系统所支持的所有命令
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }

    cout << endl;
}

void chat(int clientfd, string str)
{
    // friend:message
    int idx = str.find(":"); // 拿到: 所在的下标

    if (-1 == idx)
    {
        // 不存在冒号，说明 指令后面跟着的数据是错的

        cerr << "chat command invalid !" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string messages = str.substr(idx+1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = messages;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat msg error -> " << buffer << endl;

    }
}

// 这里默认你只要想加好友，就一定能加上，不用对方同意
void addfriend(int clientfd, string str)
{
    // friendid"
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId(); // 登录的时候保存了
    js["friendid"] = friendid;
    string buffer = js.dump(); // 序列化成字符串

    // 发送给服务器
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        // 出错
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

// 创建群组
void creategroup(int clientfd, string str)
{
    //  creategroup:groupname:groupdesc

    // 到creategroup后，指令就剩： groupname:groupdesc
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    cout << "群名：" << groupname << endl;
    string groupdesc = str.substr(idx+1, str.size()-idx);
    cout << "群描述：" << groupdesc << endl;

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    // 发送给服务器
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        // 出错
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

// 添加群组
void addgroup(int clientfd, string str)
{

    // "addgroup", "加入群组, 格式 addgroup:groupid"

    int idx = str.find(":");
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    // 发送给服务器
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        // 出错
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}

// 群聊
void groupchat(int clientfd, string str)
{
    // groupchat:groupid:message
    
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup groupchat invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string groupmsg = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = groupmsg;
    js["time"] = getCurrentTime();
    string buffer = js.dump();


    // 发送给服务器
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        // 出错
        cerr << "send addgroup msg error -> " << buffer << endl;
    }

}

// 注销，登出

void loginout(int clientfd, string str) 
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        // 出错
        cerr << "send loginout msg error -> " << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}
