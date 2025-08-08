#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend(userid, friendid) values(%d, %d)", userid, friendid);
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        mysql.update(sql);
    }
}
// 返回用户好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select u.id, u.name, u.state from User u, Friend f where (u.id = f.friendid) and f.userid = %d", userid); 
    MySQL mysql;
    vector<User> vec;
    if (mysql.connect()) // 连接成功
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查找成功
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != NULL) // 一行一行取数据
            {
                User user;
                user.setId(atoi(row[0])); // atoi()函数将字符串转换成整型
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res); // 释放资源
            return vec;
        }
    }
    return vec;
}