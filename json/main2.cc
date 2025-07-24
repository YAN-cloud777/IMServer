#include<iostream>
#include<string>
#include<jsoncpp/json/json.h>
#include<memory>
#include<sstream>

using namespace std;

void Serialize(Json::Value& value,string& dst)
{
    // Json::StreamWriterBuilder swb;
    // unique_ptr<Json::StreamWriter> writer(swb.newStreamWriter());
    // stringstream ss;
    // int ret =writer->write(value,&ss);
    // if(ret !=0 )cout<<"serialize fail"<<endl;
    // dst = ss.str();
    Json::FastWriter w;
    dst = w.write(value);
    cout<<dst<<endl;
    Json::StyledWriter s;
    dst = s.write(value);
    cout<<dst<<endl;
}

void UnSerialize(Json::Value& val ,string& src)
{
    Json::Reader r;
    r.parse(src,val);
    cout<<val["name"].asString()<<endl;
}

int main()
{

    Json::Value value;
    value["name"] = "hu";
    value["age"] = 19;
    value["score"].append(100);
    value["score"].append(99);
    value["score"].append(98);

    string ret;
    Serialize(value,ret);
    Json::Value v;
    UnSerialize(v,ret);

    return 0;
}