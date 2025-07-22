#include"../common/httplib.h"
#include<iostream>
#include<string>
using namespace std;

int main()
{
    //1:构建 Server类
    httplib::Server server;

    //2:注册回调函数
    server.Get("/hi",[](const httplib::Request& req,httplib::Response& rsp){
        cout<<req.method<<endl;
        cout<<req.path<<endl;
        for(auto head:req.headers)cout<<head.first<<" "<<head.second<<endl;
        string body  = "<html><body><h1>welcome to my channel </h1></body></html>";
        rsp.set_content(body,"test/html");
        rsp.status = 200;
    });

    //3:启动服务器
    server.listen("0.0.0.0",8080);

    return 0;
}