#include "friendmodel.hpp"
#include "db.h"





// 添加好友关系
void FriendModel :: insert(int userid, int friendid)
{
    // 1. 组装sql语句
    char sql[1024] = {0}; 

    sprintf(sql, "insert into friend values(%d, %d)", userid, friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表 friendid
vector<User> FriendModel :: query(int userid)
{
    // 1.组装SQL语句
    char sql[1024] = {0};

    // 目标：拿 userid => 找到 firendid
    // 搜到a（a约束为，与b中的朋友id相同的用户，然后b为id与函数接受到的userid相同的用户）
    // 找到后返回a的 [0] id，[1] name，[2] state
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userid);


    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid的所有离线消息，放入vec中返回
            MYSQL_ROW row;

            // 一行一行往外拿数据
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }

            // 释放资源
            mysql_free_result(res);

            return vec;
        }
    }

    // 返回为空时，表示没有离线消息
    return vec;
}