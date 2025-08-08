#include "json.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;
using json = nlohmann::json;

// json序列化
void fun(){
    json js;
    js["hello"] = "world";
    js["hi"] = "你好";
    js["vector"] = {1, 2, 3};

    string sendBuf = js.dump(); // json对象转成json字符串
    cout << sendBuf << endl;
}

void fun1(){
    json js;
    js["id"] = {1, 2, 3};
    js["name"] = {"a", "b", "c"};
    js["msg1"]["a"]  = "hello";
    js["msg1"]["b"]  = "world";

    js["msg2"] = {
        {"a", "hello"},
        {"b", "world"}
    };
    cout << js << endl;
}

void fun2(){
    // 序列化容器
    json js;
    vector<int> v = {1, 2, 3};
    js["id"] = v;

    map<int, string> m;
    m.insert({1, "黄山"}) ;
    m.insert({2, "华山"}) ;
    m.insert({3, "嵩山"}) ;
    js["name"] = m;

    cout << js << endl;
}



// json反序列化
string fun3(){
    // 序列化容器
    json js;
    vector<int> v = {1, 2, 3};
    js["id"] = v;

    map<int, string> m;
    m.insert({1, "黄山"}) ;
    m.insert({2, "华山"}) ;
    m.insert({3, "嵩山"}) ;
    js["name"] = m;
    string sendBuf = js.dump();
    return sendBuf;
}

string fun4(){
    json js;
    js["id"] = {1, 2, 3};
    js["name"] = {"a", "b", "c"};
    js["msg1"]["a"]  = "hello";
    js["msg1"]["b"]  = "world";

    js["msg2"] = {
        {"a", "hello"},
        {"b", "world"}
    };
    string sendBuf = js.dump();
    return sendBuf;
}


int main() {
    fun();
    fun1();
    fun2();

    // json反序列化： json字符串转成json对象（看作容器，方便访问）
    string recvBuf1 = fun3();
    json jsBuf1 = json::parse(recvBuf1);  // json字符串解析成json对象
    vector<int> vec = jsBuf1["id"];  // json对象的数组类型可以直接放入vector
    for (int& v : vec)
    {
        cout << v << " ";
    }
    cout << endl << jsBuf1["id"] << endl;

    map<int, string> mp = jsBuf1["name"];
    for (auto &p : mp)   // 使用引用避免拷贝，高效
    {   
        cout << p.first << ":";
        cout << p.second << " ";
    }
    cout << endl <<jsBuf1["name"] << endl;

    string recvBuf2 = fun4();
    json jsBuf2 = json::parse(recvBuf2); 
    json js = jsBuf2["msg2"];
    cout << js["a"] << endl;
    cout << js["b"] << endl;

    auto arr= jsBuf2["id"];
    cout << arr[2] << endl;
    return 0;
}