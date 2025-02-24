#include "groupmodel.hpp"
#include "groupuser.hpp"

#include "db.h"


// 创建群组
// 通过sql语句，填写数据库中的AllGroup表
bool GroupModel :: createGroup(Group &group)
{
    // 1. 组装sql语句
    char sql[1024] = {0}; 

    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    // 连接数据库，并让数据库执行sql语句
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 设置组的id ，填写数据后，数据库自动分配的
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 加入群组
void GroupModel :: addGroup(int userid, int groupid, string role)
{
    // 1. 组装sql语句
    char sql[1024] = {0}; 
    // 用户id，加入的群id，在群中扮演什么角色 存入 GroupUser表中
    sprintf(sql, "insert into groupuser(groupid,userid,grouprole) values(%d, %d, '%s')", groupid, userid, role.c_str());

    // 连接数据库，并让数据库执行sql语句
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 解散群组

// 查询用户所在群组信息
vector<Group> GroupModel :: queryGroups(int userid)
{
    /*
    1. 先根据userid在groupuser表中查询出该用户所属的群组信息
    2. 再根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    */

    char sql[1024] = {0}; 
    // 我要拿到 用户所在群组的id，名字，描述【在AllGroup】，用户所在群主的id通过查【groupuser】的userid，进而匹配到groupuserid
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join \
                groupuser b on a.id = b.groupid where b.userid=%d", userid);

    vector<Group> groupVec;
    MySQL mysql;
    if (mysql.connect())
    {
        // 拿到查询到数据
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 拿到每一行的数据
            MYSQL_ROW row;
            // 查出userid所有群组信息
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);

                groupVec.push_back(group);
            }
            // 释放资源
            mysql_free_result(res);
        }

        // 查询所在群组的其他用户【详细信息，id，名字，是否在线，在群组内扮演的角色】
        for (Group &group : groupVec)
        {
            sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a \
                    inner join groupuser b on b.userid=a.id where b.groupid=%d", group.getId());
            
            MYSQL_RES *res = mysql.query(sql);
            
            if (res != nullptr)
            {
                // 拿每一行数据
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);

                    group.getUsers().push_back(user);
                }
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他成员发送信息
// 群聊消息 =》 用户发送的消息，除了自身外的其他用户都要接受到
vector<int> GroupModel :: queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0}; 
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    vector<int> idVec;

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res;
        res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }

    return idVec;
}