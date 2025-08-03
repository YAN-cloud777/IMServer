#pragma once 
#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<atomic>
#include<random>
#include<iomanip>

namespace hu{
//生成唯一id  //生成一个由16位随机字符组成的字符串作为唯一ID
    std::string uuid()
    {
        // 1. 生成6个0~255之间的随机数字(1字节-转换为16进制字符)--生成12位16进制字符

        //具体来说 是生成一个 16字节随机字符的随机数   使用random 中的 random_device （作为种子 ，因为直接使用的话效率较低）
        //然后 用在mt19937_64算法生成对应的随机数；同时使用uniform_int_dirtribution 来限定范围  -》 生成指定范围内的随机数
        std::random_device rd;//实例化设备随机数对象-用于生成设备随机数（真随机数）
        std::mt19937 generator(rd());//以设备随机数为种子，实例化伪随机数对象（伪随机数）
        std::uniform_int_distribution<int> distribution(0,255); //限定数据范围

        std::stringstream ss;
            for (int i = 0; i < 6; i++) {// 只用生成6个 0-255 之间的随机数
                if (i == 2) ss << "-";
                ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
                //一般来说直接    ss<<std::hex << distribution(generator);即可 但是 可能出现随机数位数不够的情况 因此需要补位 
                //（std::setw(2)） 表示设置宽度为2 如果宽度不够 则以 '0' 为补充
            }
            ss << "-";
            // 2. 通过一个静态变量生成一个2字节的编号数字--生成4位16进制数字字符
            static std::atomic<short> idx(0);
            short tmp = idx.fetch_add(1);//每次增加1
            ss << std::setw(4) << std::setfill('0') << std::hex << tmp;
            return ss.str();
    }


//文件的读写操作接口
bool readFile(const std::string &filename, std::string &body){
    //实现读取一个文件的所有数据，放入body中
    std::ifstream ifs(filename, std::ios::binary | std::ios::in);
    if (ifs.is_open() == false) {
        LOG_ERROR("open file {} failed！", filename);
        return false;
    }
    ifs.seekg(0, std::ios::end);//跳转到文件末尾
    size_t flen = ifs.tellg();  //获取当前偏移量-- 文件大小
    ifs.seekg(0, std::ios::beg);//跳转到文件起始
    body.resize(flen);
    ifs.read(&body[0], flen);

//     预先分配足够内存，避免动态增长带来的性能开销；
//     使用 read(&body[0], flen) 这种方式效率高；
    if (ifs.good() == false) {
        LOG_ERROR("read file {} data failed！", filename);
        ifs.close();
        return false;
    }
    ifs.close();
    return true;
}
bool writeFile(const std::string &filename, const std::string &body){
    //实现将body中的数据，写入filename对应的文件中
    std::ofstream ofs(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (ofs.is_open() == false) {
        LOG_ERROR("open file {} failed！", filename);
        return false;
    }
    ofs.write(body.c_str(), body.size());
    if (ofs.good() == false) {
        LOG_ERROR("write file {} data failed！", filename);
        ofs.close();
        return false;
    }
    ofs.close();
    return true;
}


}


