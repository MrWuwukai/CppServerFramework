#pragma once
#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "log.h"
#include "multithread.h"

namespace Framework {

    //F from_type, T to_type
    template<class F, class T>
    class ConfigCast {
    public:
        T operator()(const F& v) {
            return boost::lexical_cast<T>(v);
        }
    };

    // std::string -> std::vector<T>
    template<class T>
    class ConfigCast<std::string, std::vector<T> > {
    public:
        // 重载函数调用运算符，用于将字符串转换为 std::vector<T>
        std::vector<T> operator()(const std::string& v) {
            // 使用 YAML 库加载字符串为 YAML 节点
            YAML::Node node = YAML::Load(v);
            // 定义一个 std::vector<T> 类型的变量 vec
            std::vector<T> vec;
            // 定义一个字符串流 ss
            std::stringstream ss;
            // 遍历 YAML 节点中的每个元素
            for (size_t i = 0; i < node.size(); ++i) {
                ss.str(""); // 清空字符串流
                ss << node[i]; // 将当前节点元素写入字符串流
                // 递归将字符串转换为 T 类型，并添加到 vec 中
                vec.push_back(ConfigCast<std::string, T>()(ss.str()));
            }
            // 返回转换后的 vector
            return vec;
            /*
            return vec;为什么可以不使用std::move？
            当函数返回一个局部变量（如 vec）时，如果满足特定条件（如返回的是纯右值或局部变量），编译器可以优化掉拷贝或移动操作（RVO/NRVO 优化），直接在调用者的栈帧上构造返回值。
            即使没有 RVO/NRVO，std::vector 也支持移动语义。如果编译器无法应用 RVO/NRVO，它会尝试调用 std::vector 的移动构造函数（而不是拷贝构造函数）来返回 vec。
            移动构造函数的代价很低（只是指针交换），所以即使没有 std::move，性能损失也很小。
            */
        }
    };

    // std::vector<T> -> std::string
    template<class T>
    class ConfigCast<std::vector<T>, std::string> {
    public:
        // 重载函数调用运算符，用于将 std::vector<T> 转换为 std::string
        std::string operator()(const std::vector<T>& v) {
            YAML::Node node; // 创建一个 YAML 节点
            // 遍历向量中的每个元素
            for (auto& i : v) {
                // 递归转换为字符串，并添加到 YAML 节点中
                node.push_back(YAML::Load(ConfigCast<T, std::string>()(i)));
            }
            std::stringstream ss; // 创建一个字符串流
            ss << node; // 将 YAML 节点输出到字符串流中
            return ss.str(); // 返回字符串流的内容
        }
    };

    // std::string -> std::list<T>
    template<class T>
    class ConfigCast<std::string, std::list<T> > {
    public:
        std::list<T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            std::list<T> vv;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i) {
                ss.str("");
                ss << node[i];
                vv.push_back(ConfigCast<std::string, T>()(ss.str()));
            }
            return vv;
        }
    };
    // std::list<T> -> std::string
    template<class T>
    class ConfigCast<std::list<T>, std::string> {
    public:
        std::string operator()(const std::list<T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node.push_back(YAML::Load(ConfigCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // std::string -> std::set<T>
    template<class T>
    class ConfigCast<std::string, std::set<T> > {
    public:
        std::set<T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            std::set<T> vv;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i) {
                ss.str("");
                ss << node[i];
                vv.insert(ConfigCast<std::string, T>()(ss.str()));
            }
            return vv;
        }
    };
    // std::set<T> -> std::string
    template<class T>
    class ConfigCast<std::set<T>, std::string> {
    public:
        std::string operator()(const std::set<T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node.push_back(YAML::Load(ConfigCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // std::string -> std::unordered_set<T>
    template<class T>
    class ConfigCast<std::string, std::unordered_set<T> > {
    public:
        std::unordered_set<T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            std::unordered_set<T> vv;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i) {
                ss.str("");
                ss << node[i];
                vv.insert(ConfigCast<std::string, T>()(ss.str()));
            }
            return vv;
        }
    };
    // std::unordered_set<T> -> std::string
    template<class T>
    class ConfigCast<std::unordered_set<T>, std::string> {
    public:
        std::string operator()(const std::unordered_set<T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node.push_back(YAML::Load(ConfigCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // std::string -> std::map<std::string, T>
    template<class T>
    class ConfigCast<std::string, std::map<std::string, T> > {
    public:
        std::map<std::string, T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            std::map<std::string, T> vv;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it) {
                ss.str("");
                ss << it->second;
                vv.insert(std::make_pair(it->first.Scalar(), ConfigCast<std::string, T>()(ss.str())));
            }
            return vv;
        }
    };
    // std::map<std::string, T> -> std::string
    template<class T>
    class ConfigCast<std::map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::map<std::string, T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node[i.first] = YAML::Load(ConfigCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // std::string -> std::unordered_map<std::string, T>
    template<class T>
    class ConfigCast<std::string, std::unordered_map<std::string, T> > {
    public:
        std::unordered_map<std::string, T> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            std::unordered_map<std::string, T> vv;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it) {
                ss.str("");
                ss << it->second;
                vv.insert(std::make_pair(it->first.Scalar(), ConfigCast<std::string, T>()(ss.str())));
            }
            return vv;
        }
    };
    // std::unordered_map<std::string, T> -> std::string
    template<class T>
    class ConfigCast<std::unordered_map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::unordered_map<std::string, T>& v) {
            YAML::Node node;
            for (auto& i : v) {
                node[i.first] = YAML::Load(ConfigCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string& name, const std::string& description = "")
            :m_name(name)
            , m_description(description) {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }
        virtual ~ConfigVarBase() {}

        const std::string& getName() const { return m_name; }
        const std::string& getDescription() const { return m_description; }
        virtual std::string getTypeName() const = 0;

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string& val) = 0;
    protected:
        std::string m_name;
        std::string m_description;
    };

    template<class T, class FromStr = ConfigCast<std::string, T>, class ToStr = ConfigCast<T, std::string> >
    class ConfigVar : public ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;

        ConfigVar(const std::string& name
            , const T& default_value
            , const std::string& description = "")
            :ConfigVarBase(name, description)
            , m_val(default_value) {
        }

        std::string getTypeName() const override {
            return typeid(T).name();
        }

        std::string toString() override {
            try {
                //return boost::lexical_cast<std::string>(m_val);
                RWMutex::ReadLock lock(m_mutex);
                return ToStr()(m_val);
            }
            catch (std::exception& e) {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception"
                    << e.what() << " convert: " << typeid(m_val).name() << " to string";
            }
            return "";
        }

        bool fromString(const std::string& val) override {
            try {
                // m_val = boost::lexical_cast<T>(val); // 简单类型转化
                setValue(FromStr()(val));
            }
            catch (std::exception& e) {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception"
                    << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        const T getValue() const { 
            // 锁会改变内部成员，这个方法不能成为常函数，除非m_mutex加上mutable修饰符
            RWMutex::ReadLock lock(m_mutex);
            return m_val; 
        }

        void setValue(const T& v) { 
            // 出这个大括号域读锁自动析构
            {
                RWMutex::ReadLock lock(m_mutex);
                if (v == m_val) {
                    return;
                }
                for (auto& i : m_cbs) {
                    i.second(m_val, v);
                }
            }
            RWMutex::WriteLock lock(m_mutex);
            m_val = v;
        }

        // 监听配置更改事件
        uint64_t addListener(on_change_cb cb) {
            static uint64_t s_fun_id = 0; // 给每个回调函数设置唯一id
            RWMutex::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }
        void delListener(uint64_t key) {
            RWMutex::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }
        void clearListener() {
            RWMutex::WriteLock lock(m_mutex);
            m_cbs.clear();
        }
        on_change_cb getListener(uint64_t key) {
            RWMutex::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }
    private:
        T m_val;
        std::map<uint64_t, on_change_cb> m_cbs;
        /*
        为什么回调函数还要包一层？
        因为functional包装的函数难以比较是否是同一函数。        
        */

        mutable RWMutex m_mutex;
    };

    class Config {
    public:
        // 定义一个映射类型，将字符串键映射到ConfigVarBase的智能指针
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        // 模板函数，用于查找或创建指定名称的配置变量
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = "") {
            // 尝试查找已存在的配置变量
            auto tmp = Lookup<T>(name);
            RWMutex::WriteLock lock(GetMutex());
            if (tmp.first != "NOT_FOUND") {
                return tmp.second;
            }

            // 检查名称是否包含无效字符
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") // ABCDEFGHIJKLMNOPQRSTUVWXYZ
                != std::string::npos) {
                // 如果包含无效字符，记录错误日志并抛出异常
                LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            // 创建新的配置变量并存储到静态数据成员中
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        // 模板函数，用于查找指定名称的配置变量（不创建新变量）
        template<class T>
        static std::pair<std::string, typename ConfigVar<T>::ptr> Lookup(const std::string& name) {
            RWMutex::ReadLock lock(GetMutex());

            // 在静态数据成员中查找配置变量
            auto it = GetDatas().find(name);
            if (it == GetDatas().end()) {
                // 如果未找到，返回 "NOT_FOUND" 状态和空指针
                return { "NOT_FOUND", nullptr };
            }
            else {
                // 尝试类型转换
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp) {
                    // 如果找到，记录日志并返回 "FOUND" 状态和指针
                    LOG_INFO(LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return { "FOUND", tmp };
                }
                else {
                    // 类型不匹配，返回错误状态和空指针
                    LOG_ERROR(LOG_ROOT()) << "Lookup name=" << name << " type wrong: " << typeid(T).name();
                    return { "TYPE_WRONG", nullptr };
                }
            }
        }

        static void LoadFromYaml(const YAML::Node& root);

        static ConfigVarBase::ptr LookupBase(const std::string& name);

        static void visit(std::function<void(ConfigVarBase::ptr)> cb);
    private:
        // 静态全局变量创建时机未知，所以需要把它包在函数里确保他先创建再使用
        static ConfigVarMap& GetDatas() {
            static ConfigVarMap s_datas;
            return s_datas;
        };
        static RWMutex& GetMutex() {
            static RWMutex s_mutex;
            return s_mutex;
        }
    };
}