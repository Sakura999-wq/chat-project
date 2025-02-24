#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 继承 User，组内的用户，用户的一个特殊分支
class GroupUser : public User
{
public:
    // 设置用户在组内的角色
    void setRole(string role) {this->role = role;}

    // 获取用户在组内的角色
    string getRole() {return this->role;}


private:
    string role;
};


#endif // !GROUPUSER_H
