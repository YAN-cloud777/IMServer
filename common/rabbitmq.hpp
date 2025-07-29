#pragma once
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#include<functional>

#include<iostream>
#include<string>


#include <iostream>
#include <functional>
#include "logger.hpp"

namespace hu
{
    class MQClient 
{
public:
        using MessageCallback = std::function<void(const char*, size_t)>;
        using ptr = std::shared_ptr<MQClient>;
        MQClient(const std::string &user, const std::string passwd,const std::string host) 
        {
            _loop = EV_DEFAULT;
            _handler = std::make_unique<AMQP::LibEvHandler>(_loop);
            //amqp://root:123456@127.0.0.1:5672/
            std::string url = "amqp://" + user + ":" + passwd + "@" + host + "/";
            AMQP::Address address(url);
            _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), address);
            _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());

            _loop_thread = std::thread([this]() {
                ev_run(_loop, 0);//由于是阻塞的线程，因此不能占用主线程 要创建多线程解决
            });
        }
        ~MQClient() {//由于不是自己的线程，不能直接回收，因此要调用异步回收，通过watcher_callback 调用ev_break(loop, EVBREAK_ALL);
             ev_async_init(&_async_watcher, watcher_callback);
             ev_async_start(_loop, &_async_watcher);
             ev_async_send(_loop, &_async_watcher);
             _loop_thread.join();
             _loop = nullptr;
        }

    //declaration of exchange and queue
    void declareComponents(const std::string& exchange,const std::string& queue,
                        const std::string &routing_key = "routing_key",AMQP::ExchangeType echange_type = AMQP::ExchangeType::direct)
    {
        // Its acutually same thing with the server.cc example
        _channel->declareExchange(exchange, echange_type)
        .onError([](const char *message) {
            LOG_ERROR("exchange declaration fail：{}", message);
            exit(0);
        })
        .onSuccess([exchange](){
            LOG_ERROR("{} exchange create success", exchange);
        });
    _channel->declareQueue(queue)
        .onError([](const char *message) {
            LOG_ERROR("Queue creation fail：{}", message);
            exit(0);
        })
        .onSuccess([queue](){
            LOG_ERROR("{} queue create success！", queue);
        });
    //6. 针对交换机和队列进行绑定
    _channel->bindQueue(exchange, queue, routing_key)
        .onError([exchange, queue](const char *message) {
            LOG_ERROR("{} - {} bind failed：", exchange, queue);
            exit(0);
        })
        .onSuccess([exchange, queue, routing_key](){
            LOG_ERROR("{} - {} - {} bind success！", exchange, queue, routing_key);
        });

    }

    bool publish(const std::string &exchange, 
            const std::string &msg, 
            const std::string &routing_key = "routing_key") {
            LOG_DEBUG("exchange {}-{} publish a message", exchange, routing_key);
            bool ret = _channel->publish(exchange,routing_key,msg );
            if (ret == false) {
                LOG_ERROR("{} publish error：", exchange);
                return false;
            }
            return true;
        }
        void consume(const std::string &queue, const MessageCallback &cb) {
            LOG_DEBUG("start booking {} queue message！", queue);
            _channel->consume(queue, "consume-tag")  //返回值 DeferredConsumer
                .onReceived([this, cb](const AMQP::Message &message, 
                    uint64_t deliveryTag, 
                    bool redelivered) {
                    cb(message.body(), message.bodySize());
                    _channel->ack(deliveryTag);
                })
                .onError([queue](const char *message){
                    LOG_ERROR("booking {} queue message error: {}", queue, message);
                    exit(0);
                });
        }


private:
        static void watcher_callback(struct ev_loop *loop, ev_async *watcher, int32_t revents) {
            ev_break(loop, EVBREAK_ALL);
        }

    struct ev_loop *_loop;
    std::unique_ptr<AMQP::LibEvHandler> _handler;
    std::unique_ptr<AMQP::TcpConnection> _connection;
    // 注意这里 需要先创建loop 再根据loop创建 handler 再根据handler 和 address创建connection 因此初始化的顺序不能错;
    std::unique_ptr<AMQP::TcpChannel> _channel;
    struct ev_async _async_watcher;//用于异步回收对应的线程
    std::thread _loop_thread;//用于创建阻塞线程（ev_run）

};
}