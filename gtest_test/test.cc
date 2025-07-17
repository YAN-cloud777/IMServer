#include"common.hpp"


int ADD(int a,int b)
{
    return a+b;
}

TEST(name,test1)
{
    ASSERT_EQ(ADD(10,20),30);
}

TEST(name,test2)
{
    //判断字符串
    string str = "HELLO";
    ASSERT_EQ(str,"hello");
}


int main(int argc,char* argv[])
{
    //命令行参数初始化
    testing::InitGoogleTest(&argc,argv);

    //开始测试
    

    return RUN_ALL_TESTS();
}