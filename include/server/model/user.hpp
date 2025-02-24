#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// 匹配User表的ORM类【类和数据映射关系】
class User
{
public:

    // 构造 User
    User(int id=-1, string name="", string password="", string state="offline")
    {
        this -> id = id;
        this -> name = name;
        this -> password = password;
        this -> state = state;
    }

    // 外部设置，内部成员变量的方法
    void setId(int id) {this -> id = id;}
    void setName(string name) {this -> name = name;}
    void setPassword(string password) {this -> password = password;}
    void setState(string state) {this -> state = state;}

    // 外部获取。内部成员变量的方法
    int getId() {return this->id;}
    string getName() {return this->name;}
    string getPassword() {return this->password;}
    string getState() {return this->state;}


private:
    int id;
    string name;
    string password;
    string state;

};


#endif // !USER_H