#pragma once
#include <string>
#include <cstddef> // std::size_t
#include <boost/date_time/posix_time/posix_time.hpp>
#include <odb/nullable.hxx>

#include <odb/core.hxx>
using namespace std;
/*
 在 C++ 中，要使用 ODB 将类声明为持久化类，需要包含 ODB 的核心头文件，并使用 #pragma db object 指令
 #pragma db object 指示 ODB 编译器将 person 类视为一个持久化类。
*/

#pragma db object // 表示这个下面的类 会被生成一张表
class Student
{
public:
    Student()
    {}

    Student( unsigned long sn, const string &name, unsigned long age,unsigned long classid)
        :_sn(sn),_name(name),_age(age),_classid(classid)
    {}//其中对于 id 为自增主键，不需要初始化

    void sn(unsigned long num)
    {
        _sn = num;
    }

    unsigned long sn()
    {
        return _sn;
    }

    void name(const string& name)
    {
        _name = name;
    }

    string name()
    {
        return _name;
    }

    void age(unsigned short age)
    {
        _age = age;
    }

    odb::nullable<unsigned short> age()
    {
        return _age;//外部自行判断是否为空
    }

    void classid(unsigned long classid)
    {
        _classid = classid;
    }

    unsigned long classid()
    {
        return _classid;
    }

    //成员的设置 和访问接口

private:    

    friend odb::access;//如果不设置这个，后面的odb 中的 access 无法访问到 成员

    #pragma db id auto // 表示将id设置为主键 和 自增
    unsigned long _id;//在生成对应的表的时候 对应的 id列名会只保留 id
    
    #pragma db unique//希望学号唯一
    unsigned long _sn;//学号

    string _name;
    
    odb::nullable<unsigned short> _age;//表示age 可以为空
    
    #pragma db index//将 classid 设置为普通索引
    unsigned long _classid;
};

#pragma db object
class Classes
{
public:

        Classes() {}
        Classes(const std::string &name) : _name(name){}
        void name(const std::string &name) { _name = name; }
        std::string name() { return _name; }
private:

    friend odb::access;//如果不设置这个，后面的odb 中的 access 无法访问到 成员

    #pragma db id auto
    unsigned long _id;

    string _name;
};

//查询所有的学生信息，并显示班级名称
#pragma db view object(Student)\
                object(Classes = classes : Student::_classes_id == classes::_id)\
                query((?))//其中的\ 表示 仍然同属一行 并且在这里 给 Classes 起别名classes 因此后面都是classes
                // 同时 对于 不同类中的成员，用 :: 进行表示 如 Student::_classes_id
struct classes_student {
    #pragma db column(Student::_id)
    unsigned long id;
    #pragma db column(Student::_sn)
    unsigned long sn;
    #pragma db column(Student::_name)
    std::string name;
    #pragma db column(Student::_age)
    odb::nullable<unsigned short>age;
    #pragma db column(classes::_name)
    std::string classes_name;
};

// 只查询学生姓名  ,   (?)  外部调用时传入的过滤条件
#pragma db view query("select name from Student" + (?))
struct all_name {
    std::string name;
};

//odb -d mysql --std c++11 --generate-query --generate-schema --profile boost/date-time student.hxx