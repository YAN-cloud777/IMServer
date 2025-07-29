#pragma once
#include <elasticlient/client.h>
#include <cpr/cpr.h>
#include <json/json.h>
#include <iostream>
#include <memory>
#include "logger.hpp"


namespace hu{
bool Serialize(const Json::Value &val, std::string &dst)
{
    //先定义Json::StreamWriter 工厂类 Json::StreamWriterBuilder 
    Json::StreamWriterBuilder swb;
    swb.settings_["emitUTF8"] = true;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    //通过Json::StreamWriter中的write接口进行序列化
    std::stringstream ss;
    int ret = sw->write(val, &ss);
    if (ret != 0) {
        std::cout << "Json反序列化失败！\n";
        return false;
    }
    dst = ss.str();
    return true;
}
bool UnSerialize(const std::string &src, Json::Value &val)
{
    Json::CharReaderBuilder crb;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    std::string err;
    bool ret = cr->parse(src.c_str(), src.c_str() + src.size(), &val, &err);
    if (ret == false) {
        std::cout << "json反序列化失败: " << err << std::endl;
        return false;
    }
    return true;
}
//json的序列化和反序列化



class ESIndex {
    public:
        ESIndex(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string &type = "_doc")
        :_name(name), _type(type), _client(client) // _name 为索引名称（相当于库） _type 表示 索引类型（type不用太关心）
        {
            Json::Value analysis;
            Json::Value analyzer;
            Json::Value ik;
            Json::Value tokenizer;//上述为ES 客户端的请求报文中的 setting部分的必须字段
            //settings.analysis.analyzer.ik.tokenizer = ik_max_word 定义使用中文分词器 IK（最大粒度分词），
            // 用于对中文内容如 nickname 进行分析。
            tokenizer["tokenizer"] = "ik_max_word";
            ik["ik"] = tokenizer;
            analyzer["analyzer"] = ik;//设置es的分词器为中文分词器
            analysis["analysis"] = analyzer;
            _index["settings"] = analysis;
        }
        ESIndex& append(const std::string &key, const std::string &type = "text",
             const std::string &analyzer = "ik_max_word", bool enabled = true) 
        {
            Json::Value fields;//字段，即要添加的 实际内容
            fields["type"] = type;//数据类型
            fields["analyzer"] = analyzer;//分词器
            if (enabled == false ) fields["enabled"] = enabled;
            _properties[key] = fields;//fileds只是properties的一个子字段  key就是要存储的字段
            return *this;
        }
        bool create(const std::string &index_id = "default_index_id") {
            Json::Value mappings;
            mappings["dynamic"] = true;
            mappings["properties"] = _properties;//正文包含在properties中
            _index["mappings"] = mappings;// mapping 是 index中的一个部分

            std::string body;
            bool ret = Serialize(_index, body);
            if (ret == false) {
                LOG_ERROR("索引序列化失败！");
                return false;
            }
            LOG_DEBUG("{}", body);
            //向es发起搜索请求
            try {
                auto rsp = _client->index(_name, _type, index_id, body);//创建索引 向es服务器注册索引
                if (rsp.status_code < 200 || rsp.status_code >= 300) {
                    LOG_ERROR("创建ES索引 {} 失败，响应状态码异常: {}", _name, rsp.status_code);
                    return false;
                }
            } catch(std::exception &e) {
                LOG_ERROR("创建ES索引 {} 失败: {}", _name, e.what());
                return false;
            }
            return true;
        }
    private:
        std::string _name;
        std::string _type;//索引类型
        Json::Value _properties;// 单独每次往里面添加的内容
        Json::Value _index;
        std::shared_ptr<elasticlient::Client> _client;
};

class ESInsert {
    public:
        ESInsert(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string &type = "_doc"):
            _name(name), _type(type), _client(client){}
        template<class T>
        ESInsert &append(const std::string &key, const T &val)
        {
            _item[key] = val;//对 item中 新增对应的key value字段
            return *this;
        }
        bool insert(const std::string id = "") {
            std::string body;
            bool ret = Serialize(_item, body);
            if (ret == false) {
                LOG_ERROR("索引序列化失败！");
                return false;
            }
            LOG_DEBUG("{}", body);
            //2. 发起搜索请求
            try {
                auto rsp = _client->index(_name, _type, id, body);
                //Elasticsearch 会把这个 JSON 内容当作一条完整的文档插入到 _name 所代表的索引中。
                //Elasticsearch 会把这个 JSON 内容当作一条完整的 document（文档），插入到 _name 所代表的 index（索引） 中

                if (rsp.status_code < 200 || rsp.status_code >= 300) {
                    LOG_ERROR("新增数据 {} 失败，响应状态码异常: {}", body, rsp.status_code);
                    return false;
                }
            } catch(std::exception &e) {
                LOG_ERROR("新增数据 {} 失败: {}", body, e.what());
                return false;
            }
            return true;
        }
    private:
        std::string _name;//对应 index的名称
        std::string _type;//对应 index的类型
        Json::Value _item;// 即对应 index中的 mapping 中的 properties 部分
        std::shared_ptr<elasticlient::Client> _client;
};

class ESRemove {
    public:
        ESRemove(std::shared_ptr<elasticlient::Client> &client, 
            const std::string &name, 
            const std::string &type = "_doc"):
            _name(name), _type(type), _client(client){}
        bool remove(const std::string &id) {
            try {
                auto rsp = _client->remove(_name, _type, id);
                if (rsp.status_code < 200 || rsp.status_code >= 300) {
                    LOG_ERROR("删除数据 {} 失败，响应状态码异常: {}", id, rsp.status_code);
                    return false;
                }
            } catch(std::exception &e) {
                LOG_ERROR("删除数据 {} 失败: {}", id, e.what());
                return false;
            }
            return true;
        }
    private:
        std::string _name;
        std::string _type;
        std::shared_ptr<elasticlient::Client> _client;
};

class ESSearch {
    public:
        ESSearch(std::shared_ptr<elasticlient::Client> &client, 
            const std::string &name, 
            const std::string &type = "_doc"):
            _name(name), _type(type), _client(client){}
        ESSearch& append_must_not_terms(const std::string &key, const std::vector<std::string> &vals) {
            Json::Value fields;
            for (const auto& val : vals){
                fields[key].append(val);
            }
            Json::Value terms;
            terms["terms"] = fields;
            _must_not.append(terms);
            return *this;
        }
        ESSearch& append_should_match(const std::string &key, const std::string &val) {
            Json::Value field;
            field[key] = val;
            Json::Value match;
            match["match"] = field;
            _should.append(match);
            return *this;
        }
        ESSearch& append_must_term(const std::string &key, const std::string &val) {
            Json::Value field;
            field[key] = val;
            Json::Value term;
            term["term"] = field;
            _must.append(term);
            return *this;
        }
        ESSearch& append_must_match(const std::string &key, const std::string &val){
            Json::Value field;
            field[key] = val;
            Json::Value match;
            match["match"] = field;
            _must.append(match);
            return *this;
        }
        Json::Value search(){
            Json::Value cond;
            if (_must_not.empty() == false) cond["must_not"] = _must_not;
            if (_should.empty() == false) cond["should"] = _should;
            if (_must.empty() == false) cond["must"] = _must;
            Json::Value query;
            query["bool"] = cond;
            Json::Value root;
            root["query"] = query;

            std::string body;
            bool ret = Serialize(root, body);
            if (ret == false) {
                LOG_ERROR("索引序列化失败！");
                return Json::Value();
            }
            LOG_DEBUG("{}", body);
            //2. 发起搜索请求
            cpr::Response rsp;
            try {
                rsp = _client->search(_name, _type, body);
                if (rsp.status_code < 200 || rsp.status_code >= 300) {
                    LOG_ERROR("检索数据 {} 失败，响应状态码异常: {}", body, rsp.status_code);
                    return Json::Value();
                }
            } catch(std::exception &e) {
                LOG_ERROR("检索数据 {} 失败: {}", body, e.what());
                return Json::Value();
            }
            //3. 需要对响应正文进行反序列化
            LOG_DEBUG("检索响应正文: [{}]", rsp.text);
            Json::Value json_res;
            ret = UnSerialize(rsp.text, json_res);
            if (ret == false) {
                LOG_ERROR("检索数据 {} 结果反序列化失败", rsp.text);
                return Json::Value();
            }
            return json_res["hits"]["hits"];
        }
    private:
        std::string _name;
        std::string _type;
        Json::Value _must_not;
        Json::Value _should;
        Json::Value _must;
        std::shared_ptr<elasticlient::Client> _client;
};
}
