#pragma once
#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "LoggerManager.h"

namespace Framework {

    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string& name, const std::string& description = "")
            :m_name(name)
            , m_description(description) {
        }
        virtual ~ConfigVarBase() {}

        const std::string& getName() const { return m_name; }
        const std::string& getDescription() const { return m_description; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string& val) = 0;
    protected:
        std::string m_name;
        std::string m_description;
    };

    template<class T>
    class ConfigVar : public ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;

        ConfigVar(const std::string& name
            , const T& default_value
            , const std::string& description = "")
            :ConfigVarBase(name, description)
            , m_val(default_value) {
        }

        std::string toString() override {
            try {
                return boost::lexical_cast<std::string>(m_val);
            }
            catch (std::exception& e) {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception"
                    << e.what() << " convert: " << typeid(m_val).name() << " to string";
            }
            return "";
        }

        bool fromString(const std::string& val) override {
            try {
                m_val = boost::lexical_cast<T>(val);
            }
            catch (std::exception& e) {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception"
                    << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        const T getValue() const { return m_val; }
        void setValue(const T& v) { m_val = v; }
    private:
        T m_val;
    };

    class Config {
    public:
        // 定义一个映射类型，将字符串键映射到ConfigVarBase的智能指针
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        // 模板函数，用于查找或创建指定名称的配置变量
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name,
            const T& default_value, const std::string& description = "") {
            // 尝试查找已存在的配置变量
            auto tmp = Lookup<T>(name);
            if (tmp) {
                // 如果找到，记录日志并返回
                LOG_INFO(LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            }

            // 检查名称是否包含无效字符
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789")
                != std::string::npos) {
                // 如果包含无效字符，记录错误日志并抛出异常
                LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            // 创建新的配置变量并存储到静态数据成员中
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            s_datas[name] = v;
            return v;
        }

        // 模板函数，用于查找指定名称的配置变量（不创建新变量）
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
            // 在静态数据成员中查找配置变量
            auto it = s_datas.find(name);
            if (it == s_datas.end()) {
                // 如果未找到，返回空指针
                return nullptr;
            }
            // 返回找到的配置变量的指针（使用dynamic_pointer_cast进行类型转换）
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

    private:
        // 静态数据成员，存储所有配置变量
        static ConfigVarMap s_datas;
    };
}