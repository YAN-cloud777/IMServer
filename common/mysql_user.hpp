#include "mysql.hpp"
#include "user.hxx"
#include "user-odb.hxx"

namespace hu {
class UserTable {
    public:
        using ptr = std::shared_ptr<UserTable>;
        UserTable(const std::shared_ptr<odb::core::database> &db):_db(db){}
        bool insert(const std::shared_ptr<User> &user) {
            try {
                odb::transaction trans(_db->begin());
                _db->persist(*user);
                trans.commit();
            }catch (std::exception &e) {
                LOG_ERROR("add new user failed {}:{}！", user->nickname(),e.what());
                return false;
            }
            return true;
        }
        bool update(const std::shared_ptr<User> &user) {
            try {
                odb::transaction trans(_db->begin());
                _db->update(*user);
                trans.commit();
            }catch (std::exception &e) {
                LOG_ERROR("Failed to update user  {}:{}！", user->nickname(), e.what());
                return false;
            }
            return true;
        }
        std::shared_ptr<User> select_by_nickname(const std::string &nickname) {
            std::shared_ptr<User> res;
            try {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;
                res.reset(_db->query_one<User>(query::nickname == nickname));
                //_db->query_one<User>(query::nickname == nickname)
                // 通过 query_one 接口进行查询 返回的是一个智能指针
                //想要修改 就必须先查询 不能直接修改
                trans.commit();
            }catch (std::exception &e) {
                LOG_ERROR("Failed to query user by nickname {}:{}！", nickname, e.what());
            }
            return res;
        }
        std::shared_ptr<User> select_by_phone(const std::string &phone) {
            std::shared_ptr<User> res;
            try {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;
                res.reset(_db->query_one<User>(query::phone == phone));
                trans.commit();
            }catch (std::exception &e) {
                LOG_ERROR("Failed to query user by phone number {}:{}！", phone, e.what());
            }
            return res;
        }
        std::shared_ptr<User> select_by_id(const std::string &user_id) {
            std::shared_ptr<User> res;
            try {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;
                res.reset(_db->query_one<User>(query::user_id == user_id));
                trans.commit();
            }catch (std::exception &e) {
                LOG_ERROR("Failed to query user by user ID {}:{}！", user_id, e.what());
            }
            return res;
        }

        //多用户查询
        std::vector<User> select_multi_users(const std::vector<std::string> &id_list) {
            // select * from user where id in ('id1', 'id2', ...)
            if (id_list.empty()) {
                return std::vector<User>();
            }
            std::vector<User> res;
            try {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;

                //需要能组织出一个语句 user_id in ('id1', 'id2', ...) 作为条件
                std::stringstream ss;
                ss << "user_id in (";
                for (const auto &id : id_list) {
                    ss << "'" << id << "',";
                }
                std::string condition = ss.str();
                condition.pop_back();
                condition += ")";
                result r(_db->query<User>(condition));
                for (result::iterator i(r.begin()); i != r.end(); ++i) {
                    res.push_back(*i);
                }
                trans.commit();
            }catch (std::exception &e) {
                LOG_ERROR("Failed to batch query users by user IDs: {}！", e.what());
            }
            return res;
        }
    private:
        std::shared_ptr<odb::core::database> _db;
};
}