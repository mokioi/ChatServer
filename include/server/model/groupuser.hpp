#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    string role;  // 角色: 群主/普通成员
};

#endif
