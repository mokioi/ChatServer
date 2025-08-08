#include "offlinemessagemodel.hpp"


// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage(userid, message) values(%d, '%s')", userid, msg.c_str());
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        mysql.update(sql);
    }
}
// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1. 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid = %d", userid);
    MySQL mysql;
    if (mysql.connect()) // 连接成功
    {
        mysql.update(sql);
    }
}
// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid = %d", userid); // 将各种类型的变量填充到sql，生成一个完整的 C 风格字符串
    MySQL mysql;
    vector<string> vec;
    if (mysql.connect()) // 连接成功
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查找成功
        {
            // 把userid用户的所有离线消息放入vec中
            MYSQL_ROW row;
            while((row= mysql_fetch_row(res)) != NULL)   // 一行一行取
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res); // 释放资源
            return vec;
        }
    }
    return vec; 
}