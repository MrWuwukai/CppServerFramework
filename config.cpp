#include "config.h"

namespace Framework {
	Config::ConfigVarMap Config::s_datas;

    /*"A.B", 10
       A:
         B: 10
         C: str
         */
    static void ListAllYamlMember(const std::string& prefix,
        const YAML::Node& node,
        std::list<std::pair<std::string, const YAML::Node> >& output) {
        // 检查配置名称是否有效（只允许包含特定字符）
        if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")
            != std::string::npos) {
            LOG_ERROR(LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        // 将当前节点添加到输出列表
        output.push_back(std::make_pair(prefix, node));
        // 如果当前节点是Map类型，递归处理其子节点
        if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                // 递归调用，为子节点构建前缀
                ListAllYamlMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
        auto it = s_datas.find(name);
        return it == s_datas.end() ? nullptr : it->second;
    }

    // 函数功能：LoadFromYaml 函数用于从 YAML::Node 中加载配置，并将配置值设置到对应的 ConfigVarBase 对象中。
    void Config::LoadFromYaml(const YAML::Node& root) {
        // 定义一个列表，用于存储所有节点（键值对）
        std::list<std::pair<std::string, const YAML::Node> > all_nodes;
        // 调用 ListAllYamlMember 函数，递归遍历 YAML 节点树，将所有节点存入 all_nodes
        ListAllYamlMember("", root, all_nodes);

        // 遍历所有节点
        for (auto& i : all_nodes) {
            // 获取当前节点的键（key）
            std::string key = i.first;
            // 如果键为空，跳过当前节点
            if (key.empty()) {
                continue;
            }

            // 将键转换为小写（可能是为了统一处理，忽略大小写差异）
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            // 根据键查找对应的 ConfigVarBase 对象
            ConfigVarBase::ptr var = LookupBase(key);

            // 如果找到了对应的 ConfigVarBase 对象
            if (var) {
                // 如果当前节点是标量（Scalar）
                if (i.second.IsScalar()) {
                    // 从字符串中加载配置值
                    var->fromString(i.second.Scalar());
                }
                else {
                    // 如果当前节点不是标量（例如是 Map 或 Sequence）
                    // 使用字符串流将节点转换为字符串
                    std::stringstream ss;
                    ss << i.second;
                    // 从字符串中加载配置值
                    var->fromString(ss.str());
                }
            }
        }
    }
}