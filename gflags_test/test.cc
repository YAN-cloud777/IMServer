#include"common.hpp"
#include<iostream>
#include<gflags/gflags.h>
DEFINE_string(ip,"127.0.01","这是服务器的IP地址");
DEFINE_int32(port,8080,"这是服务器的port");
DEFINE_bool(debug_enable,true,"这是是否开启调试");


int main(int argc,char* argv[])
{

    google::ParseCommandLineFlags(&argc,&argv,true);
    cout<<FLAGS_ip<<" "<<FLAGS_port<<" "<<FLAGS_debug_enable<<endl;
    // 上面定义的变量的名称想要真正作为全局变量使用的时候，名称前面加上FLAGS 才是全局变量的名称

    return 0;
}