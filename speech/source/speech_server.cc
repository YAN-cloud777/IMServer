//主要实现语音识别子服务的服务器的搭建
#include "speech_server.hpp"

DEFINE_string(app_id, "60694095", "Voice platform App ID");
DEFINE_string(api_key, "PWn6zlsxym8VwpBW8Or4PPGe", "Voice platform API key");
DEFINE_string(secret_key, "Bl0mn74iyAkr3FzCo5TZV7lBq7NYoms9", "Voice platform encryption key");

DEFINE_bool(run_mode, false, "Program run mode: false — Debug; true — Release；");
DEFINE_string(log_file, "", "Log output file specification in Release mode");
DEFINE_int32(log_level, 0, "Log level specification in Release mode");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd address");
DEFINE_string(base_service, "/service", "Service monitoring root directory");
DEFINE_string(instance_name, "/speech_service/instance", " instance name");
DEFINE_string(access_host, "127.0.0.1:10001", "External access address of the current instance");

DEFINE_int32(listen_port, 10001, "Rpc server listen port");
DEFINE_int32(rpc_timeout, -1, "Rpc timeout");
DEFINE_int32(rpc_threads, 1, "Number of IO threads for RPC");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    hu::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    hu::SpeechServerBuilder ssb;
    ssb.make_asr_object(FLAGS_app_id, FLAGS_api_key, FLAGS_secret_key);
    //首先必须先编写 asr_object 即 然后再 注册 到 brpc 即暴露端口 和 etcd 即 注册服务
    ssb.make_rpc_server(FLAGS_listen_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);
    ssb.make_reg_object(FLAGS_registry_host, FLAGS_base_service + FLAGS_instance_name, FLAGS_access_host);
    auto server = ssb.build();
    server->start();
    return 0;
}