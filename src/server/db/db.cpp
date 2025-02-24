#include "db.h"

#include <muduo/base/Logging.h>

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";


// 初始化数据库连接
MySQL :: MySQL ()
{
    _conn = mysql_init(nullptr); // 开辟一块用于存储资源连接的存储空间
}

// 析构，释放数据库连接资源空间
MySQL :: ~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
// 并返回判断是否连接成功
bool MySQL :: connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), 
                dbname.c_str(), 3306, nullptr, 0);
    
    if (p != nullptr) // 说明连接成功
    {
        // C和C++代码支持的是Ascll编码，如果不做这个操作，出来的Mysql中文可能会变为乱码
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success !";
    }
    else
    {
        LOG_INFO << "connect mysql error !";
    }

    return p;
}

// 更新操作 【改变数据相关】
bool MySQL :: update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << " : " << __LINE__ << " : " << sql << "更新失败 !";
        return false;
    }

    return true;
}

// 查询操作
MYSQL_RES* MySQL ::  query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << " : " << __LINE__ << " : " << sql << "查询失败 !";
        return nullptr;
    }

    return mysql_use_result(_conn);
}


// 获取连接
MYSQL* MySQL :: getConnection()
{
    return _conn;
}