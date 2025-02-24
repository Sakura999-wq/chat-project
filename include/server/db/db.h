#ifndef DB_H
#define DB_H


#include <mysql/mysql.h>
#include <string>
using namespace std;



// 数据库操作类
class MySQL
{
public:
    // 构造函数 初始化数据库连接
    MySQL();

    // 析构函数 释放数据库连接资源
    ~MySQL();


    // 连接数据库
    // 并返回判断是否连接成功
    bool connect();

    // 更新数据库 根据sql语句
    // 并返回是否更新成功
    bool update(string sql);

    // 查询操作 根据sql语句
    // 并返回判断是否查询成功
    MYSQL_RES* query(string sql);

    // 获取连接
    MYSQL* getConnection();

private:
    MYSQL *_conn;
};


#endif