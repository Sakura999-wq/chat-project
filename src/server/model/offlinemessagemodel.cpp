#include "offlinemessagemodel.hpp"
#include "db.h"
#include <string>
#include <iostream>
using namespace std;

// 存储用户的离线消息
void OfflineMsgModel ::  insert(int userid, string msg)
{
    // 1.组装SQL语句
    char sql[1024] = {0};

    sprintf(sql, "insert into offlinemessage(userid,message) values(%d, '%s')", userid, msg.c_str());
    cout << "SQL语句：" << sql << endl;

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    } 
}

// 删除用户的离线消息
void OfflineMsgModel :: remove(int userid)
{
    // 1.组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    } 
}

// 查询用户的离线消息
vector<string> OfflineMsgModel :: query(int userid)
{
    // 1.组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);


    vector<string> vec;
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
                vec.push_back(row[0]);
            }

            // 释放资源
            mysql_free_result(res);

            return vec;
        }
    }

    // 返回为空时，表示没有离线消息
    return vec;
}