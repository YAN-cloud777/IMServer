#include <brpc/server.h>
#include <butil/logging.h>
#include "main.pb.h"

//1. 继承于EchoService创建一个子类，并实现rpc调用的业务功能
class EchoServiceImpl : public example::EchoService {
    public:
        EchoServiceImpl(){}
        ~EchoServiceImpl(){}
        void Echo(google::protobuf::RpcController* controller,const ::example::EchoRequest* request
            ,::example::EchoResponse* response,::google::protobuf::Closure* done)//closure 的作用是 设置异步处理的回调函数，最后通过closure->run() 传输给 server
        {
            brpc::ClosureGuard rpc_guard(done);//同时 closureguard 的作用是类似于智能指针，用于管理 closure 类
            
            std::cout << "收到消息:" << request->message() << std::endl;

            std::string str = request->message() + "--这是响应！！";
            response->set_message(str);
            //done->Run();  这里不用传run 原因是有ClosureGuard ,这个对象会在析构的时候自动调用run函数
        }
};

int main()
{
    //关闭brpc的默认日志输出
    logging::LoggingSettings settings;
    settings.logging_dest = logging::LoggingDestination::LOG_TO_NONE;
    logging::InitLogging(settings);


    //2：创建对应的服务器类
    brpc::Server server;
    //3：向服务器类中新增服务 echoservice
    EchoServiceImpl echo_service;//就是上面定义的那个类 就是想要注册的服务
    int ret = server.AddService(&echo_service, brpc::ServiceOwnership::SERVER_DOESNT_OWN_SERVICE);//如果添加失败，brpc::Server 不会销毁这个对象；
    //     enum ServiceOwnership {
    //  //添加服务失败时，服务器将负责删除服务对象
    //  SERVER_OWNS_SERVICE,
    //  //添加服务失败时，服务器也不会删除服务对象
    //  SERVER_DOESNT_OWN_SERVICE
    // };

    if (ret == -1) {
        std::cout << "添加Rpc服务失败!\n";
        return -1;
    }
    //4：启动服务器 等待客户端访问
      brpc::ServerOptions options;
    options.idle_timeout_sec = -1; //连接空闲超时时间-超时后连接被关闭
    options.num_threads = 1; // io线程数量
    ret = server.Start(8080, &options);
    if (ret == -1) {
        std::cout << "启动服务器失败！\n";
        return -1;
    }
    server.RunUntilAskedToQuit();//修改等待运行结束
    
    return 0;
}
