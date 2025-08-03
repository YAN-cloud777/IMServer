//This module implements the voice recognition functionality.
#include <brpc/server.h>
#include <butil/logging.h>

#include "asr.hpp"      // 语音识别模块封装
#include "etcd.hpp"     // 服务注册模块封装
#include "logger.hpp"   // 日志模块封装
#include "speech.pb.h"  // protobuf框架代码


namespace hu{
class SpeechServiceImpl : public hu::SpeechService {
    public:
        SpeechServiceImpl(const ASRClient::ptr &asr_client):
            _asr_client(asr_client){}
        ~SpeechServiceImpl(){}
        void SpeechRecognition(google::protobuf::RpcController* controller,
                       const ::hu::SpeechRecognitionReq* request,
                       ::hu::SpeechRecognitionRsp* response,
                       ::google::protobuf::Closure* done) {
            LOG_DEBUG("Received speech-to-text request！");
            brpc::ClosureGuard rpc_guard(done);
            //1. 取出请求中的语音数据
            //2. 调用语音sdk模块进行语音识别，得到响应
            std::string err;
            std::string res = _asr_client->recognize(request->speech_content(), err);
            if (res.empty()) {
                LOG_ERROR("{} Speech recognition failed！", request->request_id());
                response->set_request_id(request->request_id());
                response->set_success(false);
                response->set_errmsg("Speech recognition failed:" + err);
                return;
            }
            //3. 组织响应
            response->set_request_id(request->request_id());
            response->set_success(true);
            response->set_recognition_result(res);
        }
    private:
        ASRClient::ptr _asr_client;
};

class SpeechServer {
    public:
        using ptr = std::shared_ptr<SpeechServer>;
        SpeechServer(const ASRClient::ptr asr_client, 
            const Registry::ptr &reg_client,
            const std::shared_ptr<brpc::Server> &server):
            _asr_client(asr_client),
            _reg_client(reg_client),
            _rpc_server(server){}
        ~SpeechServer(){}
        //搭建RPC服务器，并启动服务器
        void start() {
            _rpc_server->RunUntilAskedToQuit();
            //因为要先调用 make_asr_object  make_reg_object make_rpc_server 完成服务注册之后最后启动服务器
        }
    private:
        ASRClient::ptr _asr_client;
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
};

class SpeechServerBuilder {
    public:
        //构造语音识别客户端对象
        void make_asr_object(const std::string &app_id,
            const std::string &api_key,
            const std::string &secret_key) {
            _asr_client = std::make_shared<ASRClient>(app_id, api_key, secret_key);
        }
        //用于构造服务注册客户端对象
        void make_reg_object(const std::string &reg_host,
            const std::string &service_name,
            const std::string &access_host) {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, access_host);// service register
        }
        //构造RPC服务器对象
        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads) {
            if (!_asr_client) {
                LOG_ERROR("Speech recognition module not initialized！");
                abort();
            }
            _rpc_server = std::make_shared<brpc::Server>();
            SpeechServiceImpl *speech_service = new SpeechServiceImpl(_asr_client);
            int ret = _rpc_server->AddService(speech_service, 
                brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            //这里 brpc::Server::AddService() 并不是服务的 注册到服务中心，而是把服务对象 注册到 RPC 服务器内部，使其能响应远程请求（即“暴露”出接口）；
            //etcd 是用于服务发现的服务注册中心，会在后面通过 _reg_client->registry() 向 etcd 报告这个 RPC 服务的地址信息。 
            if (ret == -1) {
                LOG_ERROR("Failed to add RPC service！");
                abort();
            }
            brpc::ServerOptions options;
            options.idle_timeout_sec = timeout;
            options.num_threads = num_threads;
            ret = _rpc_server->Start(port, &options);
            if (ret == -1) {
                LOG_ERROR("Service startup failed！");
                abort();
            }
        }
        SpeechServer::ptr build() {
            if (!_asr_client) {
                LOG_ERROR("The speech recognition module has not been initialized yet！");
                abort();
            }
            if (!_reg_client) {
                LOG_ERROR("The service registration module has not been initialized yet！");
                abort();
            }
            if (!_rpc_server) {
                LOG_ERROR("The RPC server module has not been initialized yet！");
                abort();
            }
            SpeechServer::ptr server = std::make_shared<SpeechServer>(
                _asr_client, _reg_client, _rpc_server);
            return server;
        }
    private:
        ASRClient::ptr _asr_client;
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
};
}
// 需要 先创建SpeechServerBuilder 再通过 SpeechServerBuilder 来构建SpeechServer的原因：

// 构造依赖复杂（需要多个模块初始化）
// SpeechServer 依赖：

// ASRClient：语音识别模块

// Registry：服务注册模块

// brpc::Server：RPC 服务模块

// 这些模块初始化参数复杂、顺序有依赖（如 ASRClient 必须先构造，再传给 SpeechServiceImpl，后者再注册到 RPC Server），用一个构造函数很难处理清楚。

// 2. 构造过程需要逻辑控制、校验、容错
// Builder 中可以：

// 加入校验（如 _asr_client 是否构造完成）

// 输出日志和出错终止（LOG_ERROR + abort()）

// 隔离服务构造和运行逻辑（职责分离）
// 3: 保证顺序正确、对象完整地构建好
// 构造顺序很重要：


// make_asr_object()
// make_reg_object()
// make_rpc_server()
// build()
// 你必须：

// 先创建语音模块 → 再注册服务 → 再创建 RPC 服务对象 → 最后才能构造 SpeechServer

// Builder 强制这个构建顺序，防止你跳过某一步或乱序调用。