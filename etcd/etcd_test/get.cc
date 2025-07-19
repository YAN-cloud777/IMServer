#include<etcd/Client.hpp>
#include<etcd/KeepAlive.hpp>
#include<etcd/Response.hpp>
#include<etcd/Watcher.hpp>
#include<etcd/Value.hpp>
#include<thread>
#include<string>
#include<iostream>

using namespace std;


void callback(const etcd::Response &resp)
{
     if (resp.is_ok() == false) {
        std::cout << "收到一个错误的事件通知:" << resp.error_message() << std::endl;
        return;
    }
    for (auto const& ev : resp.events()) {// events类的作用是 监控kv的类型是否发生变化，如果发生变化，记录之前的值 和 现在的值
        if (ev.event_type() == etcd::Event::EventType::PUT) {
            std::cout << "服务信息发生了改变:\n" ;
            std::cout << "当前的值：" << ev.kv().key() << "-" << ev.kv().as_string() << std::endl;
            std::cout << "原来的值：" << ev.prev_kv().key() << "-" << ev.prev_kv().as_string() << std::endl;
        }else if (ev.event_type() == etcd::Event::EventType::DELETE_) {
            std::cout << "服务信息下线被删除:\n";
            std::cout << "当前的值：" << ev.kv().key() << "-" << ev.kv().as_string() << std::endl;
            std::cout << "原来的值：" << ev.prev_kv().key() << "-" << ev.prev_kv().as_string() << std::endl;
        }
    }
}

//get 的主要任务是获取数据并且通过watch 监控数据的变化
int main()
{
    // 连接etcd 服务器
    std::string etcd_host = "http://127.0.0.1:2379";

    //实例化客户端对象
    etcd::Client client(etcd_host);
    //获取键值对信息
    
    auto resp = client.ls("/service").get();//可以获取到 /service 目录下的所有键值对信息！ 可能获取到多个数据

    if (resp.is_ok() == false) {
        std::cout << "获取键值对数据失败: " << resp.error_message() << std::endl;
        return -1;
    }

    //由于可以获取到多个数据，因此 需要遍历
    int sz = resp.keys().size();
    for (int i = 0; i < sz; ++i) {
        std::cout << resp.value(i).as_string() << "可以提供" << resp.key(i) << "服务\n";
    }

    //获取watch类 用来监控键值对信息的状态变化
    auto watcher = etcd::Watcher(client,"/service",callback,true);//第一个参数是客户端，第二个参数是key value 中对应的目录 
    //watcher 可以监控对应目录下的所有kv ，第三个参数是个回调函数 即 如果状态发生变化怎么办，第四个参数是是否递归，即 service 下面的目录中的kv
    //是否也要监视 
    watcher.Wait();//阻塞等待即可

    return 0;
}