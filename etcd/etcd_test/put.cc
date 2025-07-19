#include<etcd/Client.hpp>
#include<etcd/KeepAlive.hpp>
#include<etcd/Response.hpp>
#include<etcd/Value.hpp>
#include<thread>
#include<string>
#include<iostream>

using namespace std;
int main()
{
    //0：连接上etcd服务器
    string etcd_host = "http://127.0.0.1:2379";
    //1：创建一个client对象
    etcd::Client client(etcd_host);
    //2：创建一个 keepalive 租约保活 对象，并且 设置对应有效时长的租约
    auto keepalive = client.leasekeepalive(2).get();//获取一个 对象 其中设置对应的保活时间为2s
    
    //3：获取到租约id (后面put的时候 通过 租约id 找到 租约保活对象)
    auto lease_id = keepalive->Lease();

    //4：插入数据
    auto resp = client.put("/service/usr","127.0.0.1:8080",lease_id).get();//为这个client插入的数据设置保活时间，如果client掉线，2s后对应键值对无效
    //返回的是一个response类
    if(resp.is_ok()==false)
    {
        perror("etcd put kv error");
        return -1;
    }
     auto resp2 = client.put("/service/friend", "127.0.0.1:9090").get();
    if (resp2.is_ok() == false) {
        std::cout << "新增数据失败：" << resp2.error_message() << std::endl;
        return -1;
    }



    return 0;
}