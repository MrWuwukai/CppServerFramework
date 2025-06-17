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
        // ���غ�����������������ڽ��ַ���ת��Ϊ std::vector<T>
        std::vector<T> operator()(const std::string& v) {
            // ʹ�� YAML ������ַ���Ϊ YAML �ڵ�
            YAML::Node node = YAML::Load(v);
            // ����һ�� std::vector<T> ���͵ı��� vec
            std::vector<T> vec;
            // ����һ���ַ����� ss
            std::stringstream ss;
            // ���� YAML �ڵ��е�ÿ��Ԫ��
            for (size_t i = 0; i < node.size(); ++i) {
                ss.str(""); // ����ַ�����
                ss << node[i]; // ����ǰ�ڵ�Ԫ��д���ַ�����
                // �ݹ齫�ַ���ת��Ϊ T ���ͣ�����ӵ� vec ��
                vec.push_back(ConfigCast<std::string, T>()(ss.str()));
            }
            // ����ת����� vector
            return vec;
            /*
            return vec;Ϊʲô���Բ�ʹ��std::move��
            ����������һ���ֲ��������� vec��ʱ����������ض��������緵�ص��Ǵ���ֵ��ֲ��������������������Ż����������ƶ�������RVO/NRVO �Ż�����ֱ���ڵ����ߵ�ջ֡�Ϲ��췵��ֵ��
            ��ʹû�� RVO/NRVO��std::vector Ҳ֧���ƶ����塣����������޷�Ӧ�� RVO/NRVO�����᳢�Ե��� std::vector ���ƶ����캯���������ǿ������캯���������� vec��
            �ƶ����캯���Ĵ��ۺܵͣ�ֻ��ָ�뽻���������Լ�ʹû�� std::move��������ʧҲ��С��
            */
        }
    };

    // std::vector<T> -> std::string
    template<class T>
    class ConfigCast<std::vector<T>, std::string> {
    public:
        // ���غ�����������������ڽ� std::vector<T> ת��Ϊ std::string
        std::string operator()(const std::vector<T>& v) {
            YAML::Node node; // ����һ�� YAML �ڵ�
            // ���������е�ÿ��Ԫ��
            for (auto& i : v) {
                // �ݹ�ת��Ϊ�ַ���������ӵ� YAML �ڵ���
                node.push_back(YAML::Load(ConfigCast<T, std::string>()(i)));
            }
            std::stringstream ss; // ����һ���ַ�����
            ss << node; // �� YAML �ڵ�������ַ�������
            return ss.str(); // �����ַ�����������
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
                // m_val = boost::lexical_cast<T>(val); // ������ת��
                setValue(FromStr()(val));
            }
            catch (std::exception& e) {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception"
                    << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        const T getValue() const { 
            // ����ı��ڲ���Ա������������ܳ�Ϊ������������m_mutex����mutable���η�
            RWMutex::ReadLock lock(m_mutex);
            return m_val; 
        }

        void setValue(const T& v) { 
            // �����������������Զ�����
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

        // �������ø����¼�
        uint64_t addListener(on_change_cb cb) {
            static uint64_t s_fun_id = 0; // ��ÿ���ص���������Ψһid
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
        Ϊʲô�ص�������Ҫ��һ�㣿
        ��Ϊfunctional��װ�ĺ������ԱȽ��Ƿ���ͬһ������        
        */

        mutable RWMutex m_mutex;
    };

    class Config {
    public:
        // ����һ��ӳ�����ͣ����ַ�����ӳ�䵽ConfigVarBase������ָ��
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        // ģ�庯�������ڲ��һ򴴽�ָ�����Ƶ����ñ���
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = "") {
            // ���Բ����Ѵ��ڵ����ñ���
            auto tmp = Lookup<T>(name);
            RWMutex::WriteLock lock(GetMutex());
            if (tmp.first != "NOT_FOUND") {
                return tmp.second;
            }

            // ��������Ƿ������Ч�ַ�
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") // ABCDEFGHIJKLMNOPQRSTUVWXYZ
                != std::string::npos) {
                // ���������Ч�ַ�����¼������־���׳��쳣
                LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            // �����µ����ñ������洢����̬���ݳ�Ա��
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        // ģ�庯�������ڲ���ָ�����Ƶ����ñ������������±�����
        template<class T>
        static std::pair<std::string, typename ConfigVar<T>::ptr> Lookup(const std::string& name) {
            RWMutex::ReadLock lock(GetMutex());

            // �ھ�̬���ݳ�Ա�в������ñ���
            auto it = GetDatas().find(name);
            if (it == GetDatas().end()) {
                // ���δ�ҵ������� "NOT_FOUND" ״̬�Ϳ�ָ��
                return { "NOT_FOUND", nullptr };
            }
            else {
                // ��������ת��
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp) {
                    // ����ҵ�����¼��־������ "FOUND" ״̬��ָ��
                    LOG_INFO(LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return { "FOUND", tmp };
                }
                else {
                    // ���Ͳ�ƥ�䣬���ش���״̬�Ϳ�ָ��
                    LOG_ERROR(LOG_ROOT()) << "Lookup name=" << name << " type wrong: " << typeid(T).name();
                    return { "TYPE_WRONG", nullptr };
                }
            }
        }

        static void LoadFromYaml(const YAML::Node& root);

        static ConfigVarBase::ptr LookupBase(const std::string& name);

        static void visit(std::function<void(ConfigVarBase::ptr)> cb);
    private:
        // ��̬ȫ�ֱ�������ʱ��δ֪��������Ҫ�������ں�����ȷ�����ȴ�����ʹ��
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