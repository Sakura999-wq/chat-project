# chat-project
这是一个可以在Nginx TCP负载均衡环境下运行的集群聊天服务器和客户端源码项目，基于muduo实现，并结合了Redis和MySQL

编译方式：
在chat-project文件夹下，输入：./autobuild.sh

运行方式：
在 /chat-project/bin下：
启动服务器：
./ChatServer 127.0.0.1 6000
./ChatServer 127.0.0.1 6002
启动客户端：
./ChatClient 127.0.0.1 8000
./ChatClient 127.0.0.1 8000
...

根据终端所显示的提示，进行聊天
