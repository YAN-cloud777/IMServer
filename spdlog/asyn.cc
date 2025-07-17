#include"common.hpp"
#include<spdlog/sinks/stdout_color_sinks.h>//stdout_color_mt 只有在包含这个头文件之后才能正常使用
#include<spdlog/sinks/basic_file_sink.h>//basic_logger_mt 只有在包含这个头文件之后才能正常使用
#include<spdlog/async.h>
void test_log()
{
    //设置全局的刷新策略   或者可以  单独的对每个日志器创建自己的刷新策略
    //对于全局的刷新策略
    spdlog::flush_every(std::chrono::seconds(1));//每秒进行输出一次
    spdlog::flush_on(spdlog::level::level_enum::debug);//遇到debug 等级以上的日志 立即刷新

    //设置日志的输出等级（每个日志可以单独进行设置）
    spdlog::set_level(spdlog::level::level_enum::debug);//只输出debug 等级以上的日志
    
    //创建对应的日志器
    //auto logger = spdlog::stdout_color_mt("default log");//此时是直接往标准输出中打印
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>("file logger","async.log");
    //模版参数默认是同步工厂，因此在这里改成异步工厂创建的就是异步日志器

    //如果没有设置全局的刷新策略，可以在这里单独设计日志器刷新策略
    //logger -> flush_every(std::chrono::seconds(1));
    // logger -> set_level(spdlog::level::level_enum::debug);//只输出debug 等级以上的日志

    //确定日志的输出格式
    logger->set_pattern("[%n] %H:%M:%S [%t] [%-7l] %v");// - 表示左对齐 7表示占七个字符位置  [%n] 表示日志器名称

    //输出日志
    logger->debug("你好，{}","小明");
    logger->critical("你好，{}","小明");
 

}


int main()
{
    test_log();

    return 0;
}