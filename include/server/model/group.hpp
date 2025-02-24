#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"

#include <string>
#include <vector>
using namespace std;


// User表的ORM类
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    // 设置
    void setId(int id) {this -> id = id;}
    void setName(string name) {this -> name = name;}
    void setDesc(string desc) {this -> desc = desc;}


    // 获取
    int getId() {return this->id;}
    string getName() {return this->name;}
    string getDesc() {return this->desc;}

    // 获取组内成员
    vector<GroupUser> &getUsers() {return this->users;}

private:
    int id;
    string name;
    string desc;

    // 群主内的用户
    vector<GroupUser> users;
};




#endif // !GROUP_H
