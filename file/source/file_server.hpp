//1:创建对应的rpc 服务类 -- 实现rpc的业务处理接口
//2:创建 文件服务的服务器类
//3:创建 文件服务的build 类

#include <brpc/server.h>
#include <butil/logging.h>

#include "etcd.hpp"     // 服务注册模块封装
#include "logger.hpp"   // 日志模块封装
#include "utils.hpp"// 其中包含了实现 文件唯一id的生成 和  读取 和 上传文件数据的类方法
#include "base.pb.h"
#include "file.pb.h"

namespace hu{
class FileServiceImpl : public hu::FileService {// brpc中 将实现的功能业务接口（就是在file.proto 中定义的 那些services） 放到了FileService 中，是纯虚函数必须要重写
    public:
        FileServiceImpl(const std::string &storage_path):
            _storage_path(storage_path){
            umask(0);
            mkdir(storage_path.c_str(), 0775);
            if (_storage_path.back() != '/') _storage_path.push_back('/');
        }
        ~FileServiceImpl(){}

//只有先上传文件，服务器才会生成一个 fid（通常是 uuid），并把它返回给你。之后你想下载这个文件时，就必须使用这个 fid。



        //单文件的下载
        void GetSingleFile(google::protobuf::RpcController* controller,
                    const ::hu::GetSingleFileReq* request,
                    ::hu::GetSingleFileRsp* response,
                    ::google::protobuf::Closure* done) {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            //1. 取出请求中的文件ID（起始就是文件名）
            std::string fid = request->file_id();
            std::string filename = _storage_path + fid;
            //2. 将文件ID作为文件名，读取文件数据
            std::string body;
            bool ret = readFile(filename, body);
            if (ret == false) {
                response->set_success(false);
                response->set_errmsg("Read file data failed！");
                LOG_ERROR("{} Read file data failed！", request->request_id());
                return;
            }
            //3. 组织响应
            response->set_success(true);
            response->mutable_file_data()->set_file_id(fid);
            response->mutable_file_data()->set_file_content(body);
        }

        //多文件的下载
        void GetMultiFile(google::protobuf::RpcController* controller,
                    const ::hu::GetMultiFileReq* request,
                    ::hu::GetMultiFileRsp* response,
                    ::google::protobuf::Closure* done) {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            // 循环取出请求中的文件ID，读取文件数据进行填充
            for (int i = 0; i < request->file_id_list_size(); i++) {
                std::string fid = request->file_id_list(i);
                std::string filename = _storage_path + fid;
                std::string body;
                bool ret = readFile(filename, body);
                if (ret == false) {
                    response->set_success(false);
                    response->set_errmsg("Failed to read file data！");
                    LOG_ERROR("{} Failed to read file data！", request->request_id());
                    return;
                }
                FileDownloadData data;
                data.set_file_id(fid);
                data.set_file_content(body);
                response->mutable_file_data()->insert({fid, data});
            }
            response->set_success(true);
        }

        //上传单文件
        void PutSingleFile(google::protobuf::RpcController* controller,
                    const ::hu::PutSingleFileReq* request,
                    ::hu::PutSingleFileRsp* response,
                    ::google::protobuf::Closure* done) {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            //1. 为文件生成一个唯一uudi作为文件名 以及 文件ID
            std::string fid = uuid();
            std::string filename = _storage_path + fid;
            //2. 取出请求中的文件数据，进行文件数据写入
            bool ret = writeFile(filename, request->file_data().file_content());
            if (ret == false) {
                response->set_success(false);
                response->set_errmsg("Failed to read file data！");
                LOG_ERROR("{} Failed to write file data.！", request->request_id());
                return;
            }
            //3. 组织响应
            response->set_success(true);
            response->mutable_file_info()->set_file_id(fid);
            response->mutable_file_info()->set_file_size(request->file_data().file_size());
            response->mutable_file_info()->set_file_name(request->file_data().file_name());
        }


        //上传多文件
        void PutMultiFile(google::protobuf::RpcController* controller,
                    const ::hu::PutMultiFileReq* request,
                    ::hu::PutMultiFileRsp* response,
                    ::google::protobuf::Closure* done) {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            for (int i = 0; i < request->file_data_size(); i++) {
                std::string fid = uuid();
                std::string filename = _storage_path + fid;
                bool ret = writeFile(filename, request->file_data(i).file_content());
                if (ret == false) {
                    response->set_success(false);
                    response->set_errmsg("Failed to write file data.！");
                    LOG_ERROR("{} Failed to read file data.！", request->request_id());
                    return;
                }
                hu::FileMessageInfo *info  = response->add_file_info();
                info->set_file_id(fid);
                info->set_file_size(request->file_data(i).file_size());
                info->set_file_name(request->file_data(i).file_name());
            }
            response->set_success(true);
        }
    private:
        std::string _storage_path;
};

class FileServer {
    public:
        using ptr = std::shared_ptr<FileServer>;
        FileServer(const Registry::ptr &reg_client,
            const std::shared_ptr<brpc::Server> &server):
            _reg_client(reg_client),
            _rpc_server(server){}
        ~FileServer(){}
        //搭建RPC服务器，并启动服务器
        void start() {
            _rpc_server->RunUntilAskedToQuit();
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
};

class FileServerBuilder {
    public:
        //用于构造服务注册客户端对象
        void make_reg_object(const std::string &reg_host,
            const std::string &service_name,
            const std::string &access_host) {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, access_host);
        }
        //构造RPC服务器对象
        void make_rpc_server(uint16_t port, int32_t timeout, 
            uint8_t num_threads, const std::string &path = "./data/") {
            _rpc_server = std::make_shared<brpc::Server>();
            FileServiceImpl *file_service = new FileServiceImpl(path);
            int ret = _rpc_server->AddService(file_service, 
                brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret == -1) {
                LOG_ERROR("Failed to add RPC service！");
                abort();
            }
            brpc::ServerOptions options;
            options.idle_timeout_sec = timeout;
            options.num_threads = num_threads;
            ret = _rpc_server->Start(port, &options);
            if (ret == -1) {
                LOG_ERROR("Failed to start service！");
                abort();
            }
        }
        FileServer::ptr build() {
            if (!_reg_client) {
                LOG_ERROR("The service registry module has not been initialized！");
                abort();
            }
            if (!_rpc_server) {
                LOG_ERROR("The RPC server module has not been initialized yet!！");
                abort();
            }
            FileServer::ptr server = std::make_shared<FileServer>(_reg_client, _rpc_server);
            return server;
        }
    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
};
}