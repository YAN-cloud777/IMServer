#pragma once
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Response.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/Value.hpp>
#include <functional>
#include "logger.hpp"

namespace bite_im{
//服务注册客户端类
class Registry {
    public:
        using ptr = std::shared_ptr<Registry>;
        Registry(const std::string &host):
            _client(std::make_shared<etcd::Client>(host)) ,
            _keep_alive(_client->leasekeepalive(3).get()),
            _lease_id(_keep_alive->Lease()){}
        ~Registry() { _keep_alive->Cancel(); }
        bool registry(const std::string &key, const std::string &val) //此处是实例的外部访问地址，也就是实际上的服务器地址
        {
            auto resp = _client->put(key, val, _lease_id).get();
            if (resp.is_ok() == false) {
                LOG_ERROR("注册服务出现失败/registraion error: {}", resp.error_message());
                return false;
            }
            return true;
        }
    private:
        std::shared_ptr<etcd::Client> _client;
        std::shared_ptr<etcd::KeepAlive> _keep_alive;//和test中一样 keep_alive 是依赖于 client的 因此要先定义client再来定义keepalive
        uint64_t _lease_id;//同理，只有先定义了keep_alive 才能知道对应的lease id 是一个相互依赖的关系
};

//服务发现客户端类
class Discovery {
    public:
        using ptr = std::shared_ptr<Discovery>;
        using NotifyCallback = std::function<void(std::string, std::string)>;
        Discovery(const std::string &host, const std::string &basedir,const NotifyCallback &put_cb,const NotifyCallback &del_cb)
            :_client(std::make_shared<etcd::Client>(host)) 
            ,_put_cb(put_cb)
            , _del_cb(del_cb)
            {
            //先进行服务发现,先获取到当前已有的数据
            auto resp = _client->ls(basedir).get();
            if (resp.is_ok() == false) {
                LOG_ERROR("获取服务注册信息失败：{}", resp.error_message());
            }
            int sz = resp.keys().size();
            for (int i = 0; i < sz; ++i) {
                if (_put_cb) _put_cb(resp.key(i), resp.value(i).as_string());
            }
            //然后进行事件监控，监控数据发生的改变并调用回调进行处理
            _watcher = std::make_shared<etcd::Watcher>(*_client.get(), basedir,std::bind(&Discovery::callback, this, std::placeholders::_1), true);
            //造函数要求传入的是 etcd::Client& 类型（引用），因此必须解引用 shared_ptr; basedir 是服务注册 key 的前缀，表示只监听指定业务目录
        }
        ~Discovery() {
            _watcher->Cancel();
        }
    private:
        void callback(const etcd::Response &resp) {
            if (resp.is_ok() == false) {
                LOG_ERROR("收到一个错误的事件通知: {}", resp.error_message());
                return;
            }
            for (auto const& ev : resp.events()) {
                if (ev.event_type() == etcd::Event::EventType::PUT) {
                    if (_put_cb) _put_cb(ev.kv().key(), ev.kv().as_string());
                    LOG_DEBUG("新增服务：{}-{}", ev.kv().key(), ev.kv().as_string());
                }else if (ev.event_type() == etcd::Event::EventType::DELETE_) {
                    if (_del_cb) _del_cb(ev.prev_kv().key(), ev.prev_kv().as_string());
                    LOG_DEBUG("下线服务：{}-{}", ev.prev_kv().key(), ev.prev_kv().as_string());
                }
            }
        }
    private:
        NotifyCallback _put_cb;
        NotifyCallback _del_cb;
        std::shared_ptr<etcd::Client> _client;
        std::shared_ptr<etcd::Watcher> _watcher;
};
}