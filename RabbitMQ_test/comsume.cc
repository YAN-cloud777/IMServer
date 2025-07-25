
#include<iostream>
#include<string>
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>



//消息回调处理函数(notes and pdf 中 有说明)
void MessageCb(AMQP::TcpChannel *channel, const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
{
    std::string msg;
    msg.assign(message.body(), message.bodySize());//body的类型是const char* 因此默认没有/0 如果直接打印会出问题
    std::cout << msg << std::endl;
    channel->ack(deliveryTag); // 对消息标识为deliveryTag的消息 进行确认 
    // 如果消息没有ack RabbitMQ服务器即使消息消费成功 也不会把消息删除
}

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
    AMQP::Address address("amqp://root:123456@127.0.0.1:5672/");
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

    //7. 订阅队列消息 -- 设置消息处理回调函数
    auto callback = std::bind(MessageCb, &channel, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    channel.consume("test_queue", "consume-tag")  //返回值 DeferredConsumer
        .onReceived(callback)//onError 之后返回的是AMQP::Deferred父类 其中不能继续调用onReceived（没有对应接口），因此要先调用onReceived 其返回值是DeferredConsumer 子类 可以继续调用
        .onError([](const char *message){
            std::cout << "Got Message from queue error :" << message << std::endl;
            exit(0);
        }); // 返回值是 AMQP::Deferred

    // 8：休眠 + 释放资源
    ev_run(loop, 0);//启动网络通信框架(开启io)(阻塞接口)

    return 0;
}