#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>

#include<iostream>
#include<string>

int main()
{

    // 1：实例化网络通信IO 事件监控句柄
    // access to the event loop
    auto *loop = EV_DEFAULT;
    // 2：实例化 libevhandler 句柄（作用是把AMQP 和 事件监控关联起来）
    AMQP::LibEvHandler handler(loop);

    // 2：实例化连接对象（libev.cpp 中有例子）
    // // make a connection
    //     AMQP::Address address("amqp://guest:guest@localhost/");
    // //    AMQP::Address address("amqps://guest:guest@localhost/");
    //     AMQP::TcpConnection connection(&handler, address);
    AMQP::Address address("amqp://root:123456@127.0.0.1:5672/");//注意RabbitMQ 的 AMQP 协议端口应为 5672（而不是 15672），
                                                                //15672 是用于 Web 管理界面访问的端口。
    AMQP::TcpConnection connection(&handler, address);

    // 3：实例化channel对象
    AMQP::TcpChannel channel(&connection);

    // 4：使用channel 对象 声明交换机 这里声明的绑定规则是direct 即 直接绑定
    channel.declareExchange("test_exchange", AMQP::ExchangeType::direct)
            .onError([](const char *message) {
                std::cout << "exchange declaration fail：" << message << std::endl;
                exit(0);
            })
            .onSuccess([](){
                std::cout << "test-exchange create success！" << std::endl;
            });

    // 5：声明 队列
    channel.declareQueue("test_queue")
            .onError([](const char *message) {
                std::cout << "Queue creation fail：" << message << std::endl;
                exit(0);
            })
            .onSuccess([](){
                std::cout << "test_queue create success" << std::endl;
            });

    // 6：将 交换机 和 队列进行绑定   交换机名称 队列名称 和 routing key(此时是直接绑定 routing key作用不大)
    channel.bindQueue("test_exchange", "test_queue", "test-queue-key")
            .onError([](const char *message) {
                std::cout << "test-exchange - test-queue binding fail" << message << std::endl;
                exit(0);
            })
            .onSuccess([](){
                std::cout << "test-exchange - test-queue binding success" << std::endl;
            });
    // 7：向交换机中发布消息  即publish的时候 1：哪个交换机，2：什么routing key 3：什么message
    for (int i = 0; i < 10; i++) {
            std::string msg = "You got one message NO." + std::to_string(i);
            bool ret = channel.publish("test_exchange", "test-queue-key", msg);
            if (ret == false) {
                std::cout << "publish failed\n";
            }
        }
    // 8：休眠 + 释放资源
    ev_run(loop, 0);//启动网络通信框架(开启io)(阻塞接口)

    return 0;
}