#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

// User表的增加方法
bool UserModel::insert(User &user)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        if (mysql.update(sql)) // 更新成功
        {
            // 获取插入成功的用户数据生成的主键，mysql_insert_id获取最近一次 INSERT 操作为 AUTO_INCREMENT（自增）列所生成的 ID 值
            user.setId(mysql_insert_id(mysql.getConnection())); // 将内存中的user的id更新（表中id自增，但是初始化的id未改变）
            return true;
        }
    }
    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id = %d", id); // 将各种类型的变量填充到sql，生成一个完整的 C 风格字符串
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查找成功
        {
            MYSQL_ROW row = mysql_fetch_row(res); // 从结果集res中取出下一行数据
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0])); // atoi()函数将字符串转换成整型
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res); // 释放资源
                return user;
            }
        }
    }
    return User(); // 返回默认用户
}

// 更新用户状态信息
bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "update User set state = '%s' where id = %d", user.getState().c_str(), user.getId()); // %s：char*类型，c_str()将string转成char*
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        if (mysql.query(sql) != nullptr) // 更新成功
        {
            return true;
        }
    }
    return false;
}

// 重置用户的状态信息
void UserModel::resetState()
{
    char sql[1024] = "update User set state = 'offline' where state = 'online'";
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}