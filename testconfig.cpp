#include "config.h"
#include "LoggerManager.h"
#include <yaml-cpp/yaml.h>

Framework::ConfigVar<int>::ptr g_int_value_config = Framework::Config::Lookup("system.port", (int)8080, "system port");
Framework::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = Framework::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if (node.IsNull()) {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    }
    else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("./log.yml");
    print_yaml(root, 0);
    //LOG_INFO(LOG_ROOT()) << root;
}

void test_yaml1() {
    YAML::Node root = YAML::LoadFile("./log.yml");
    Framework::Config::LoadFromYaml(root);
    LOG_INFO(LOG_ROOT()) << g_int_value_config->getValue();
    LOG_INFO(LOG_ROOT()) << g_int_value_config->toString();
}

void test_yaml2() {
    YAML::Node root = YAML::LoadFile("./log.yml");
    Framework::Config::LoadFromYaml(root);
    auto v = g_int_vec_value_config->getValue();
    for (auto& i : v) {
        LOG_INFO(LOG_ROOT()) << "int_vec: " << i;
    }
}

int main(int argc, char** argv) {
    //test_yaml();
    //test_yaml1();
    test_yaml2();
    return 0;
}