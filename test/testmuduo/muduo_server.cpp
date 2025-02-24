/*
muduo 网络库给用户提供了两个主要的类
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

epoll + 线程池
好处：把网络I/O的代码和，业务代码区分开，能够专注于业务逻辑的编写
*/

// g++ -o server -muduo_server.cpp -lmuduo_net -lmuduo_base -lpthread
// telnet
/* 
退出：ctrl + ] 
telnet> quit 
*/

// 登录mysql：sudo mysql -u root -p
// 显示有什么数据库：show databases;
// 使用某个数据库：use 数据库名
// 显示数据库中有几张表：

/* Ctrl + Shift + B 可以直接build*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream> 
#include <functional>
#include <string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*基于muduo网络库开发服务器程序
1.组合 TcpServer对象
2.创建 EventLoop事件循环对象的指针
3.明确 TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置合适的服务端线程数量，muduo库会自行划分I/O线程和worker线程
*/

class ChatServer
{
public:
    ChatServer(EventLoop* loop, // 事件循环
        const InetAddress& listenAddr,  // IP + Port
        const string& nameArg)  // 服务器的名字
        : _server(loop, listenAddr, nameArg),_Loop(loop)
    {
        // 给服务器注册用户 连接的创建和断开 回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // 绑定onConnection函数，并说明有一个参数

        // 给服务器注册用户读写事件 回调
        _server.setMessageCallback(std::bind(&ChatServer::OnMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量 1个I/o线程 3个worker线程
        _server.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门处理用户的连接创建、断开 --- 注册到muduo库上，让muduo库自己去判断用户的连接和创建
    void onConnection(const TcpConnectionPtr &conn)
    {

        
        // 判断是否连接成功
        if (conn -> connected())
        {
            // 连接成功 peerAddress：服务端，localAddress客户端
            cout << conn -> peerAddress().toIpPort() << " -> " <<
                conn -> localAddress().toIpPort() << 
                " state:online" << endl;
        }
        else
        {
            // 未连接成功
            cout << conn -> peerAddress().toIpPort() << " -> " <<
                conn -> localAddress().toIpPort() << 
                " state:offline" << endl;
            
            conn -> shutdown(); // 释放连接 close(fd)
            // _loop -> quit() // 停止服务器继续给客户端使用
        }
    }

    // 专门处理用户的读写事件
    void OnMessage (const TcpConnectionPtr &conn,  // 连接
        Buffer *buffer, // 缓冲区
        Timestamp time)  // 接收数据的时间信息
    {
        string buf = buffer -> retrieveAllAsString(); // 接收缓存区内所有数据
        cout << "recv data：" << buf << "time：" << time.toString() << endl;

        // 将接收到的数据，重新发送回去
        conn -> send(buf);
    }


    TcpServer _server;
    EventLoop *_Loop;
};



int main()
{
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl=epoll
    loop.loop(); // epoll_wait 以阻塞方式等待新用户连接，或已连接用户的读写事件操作

    return 0;    
}
