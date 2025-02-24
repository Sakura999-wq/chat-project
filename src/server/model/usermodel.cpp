#include "usermodel.hpp"
#include "db.h"

#include <iostream>

// User表的增加方法
bool UserModel :: insert(User &user)
{
    // 1. 组装sql语句
    char sql[1024] = {0}; 
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
        user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 插入成功

            // 获取插入成功的用户数据生成的主键id，并设置给user
            user.setId(mysql_insert_id(mysql.getConnection()));


            return true;
        }
    }

    // 插入失败
    return false;
}


// 根据用户号码查询用户信息
User UserModel :: query(int id)
{
    // 1. 组装sql语句
    char sql[1024] = {0}; 
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 查询用户数据 user
        if (res != nullptr)
        {
            // 查询成功
            MYSQL_ROW row = mysql_fetch_row(res); // 将整行数据返回

            if (row != nullptr)
            {
                // 表示这行内有数据
                User user;
                user.setId(atoi(row[0])); // id：atoi将获取到的字符串数据转为整数
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);

                // 释放资源
                mysql_free_result(res);
                return user;
            }
        }
    }

    // 没有找到，返回默认的User，表示出错
    return User();

}


// 更新用户的状态信息
bool UserModel :: updateState(User user)
{
    // 1. 组装sql语句
    char sql[1024] = {0}; 
    
    sprintf(sql, "update user set State = '%s' where id = %d", user.getState().c_str(), user.getId());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }

    // 更新失败
    return false;
}

// 重置用户的状态信息
void UserModel::resetState()
{
    // 1. 组装sql语句
    char sql[1024] = {0}; 
    // 将 online用户全部更新为offline
    sprintf(sql, "update user set state='offline' where state='online'");

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}