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
        // ������������Ƿ���Ч��ֻ��������ض��ַ���
        if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")
            != std::string::npos) {
            LOG_ERROR(LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        // ����ǰ�ڵ���ӵ�����б�
        output.push_back(std::make_pair(prefix, node));
        // �����ǰ�ڵ���Map���ͣ��ݹ鴦�����ӽڵ�
        if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                // �ݹ���ã�Ϊ�ӽڵ㹹��ǰ׺
                ListAllYamlMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
        auto it = s_datas.find(name);
        return it == s_datas.end() ? nullptr : it->second;
    }

    // �������ܣ�LoadFromYaml �������ڴ� YAML::Node �м������ã���������ֵ���õ���Ӧ�� ConfigVarBase �����С�
    void Config::LoadFromYaml(const YAML::Node& root) {
        // ����һ���б����ڴ洢���нڵ㣨��ֵ�ԣ�
        std::list<std::pair<std::string, const YAML::Node> > all_nodes;
        // ���� ListAllYamlMember �������ݹ���� YAML �ڵ����������нڵ���� all_nodes
        ListAllYamlMember("", root, all_nodes);

        // �������нڵ�
        for (auto& i : all_nodes) {
            // ��ȡ��ǰ�ڵ�ļ���key��
            std::string key = i.first;
            // �����Ϊ�գ�������ǰ�ڵ�
            if (key.empty()) {
                continue;
            }

            // ����ת��ΪСд��������Ϊ��ͳһ�������Դ�Сд���죩
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            // ���ݼ����Ҷ�Ӧ�� ConfigVarBase ����
            ConfigVarBase::ptr var = LookupBase(key);

            // ����ҵ��˶�Ӧ�� ConfigVarBase ����
            if (var) {
                // �����ǰ�ڵ��Ǳ�����Scalar��
                if (i.second.IsScalar()) {
                    // ���ַ����м�������ֵ
                    var->fromString(i.second.Scalar());
                }
                else {
                    // �����ǰ�ڵ㲻�Ǳ����������� Map �� Sequence��
                    // ʹ���ַ��������ڵ�ת��Ϊ�ַ���
                    std::stringstream ss;
                    ss << i.second;
                    // ���ַ����м�������ֵ
                    var->fromString(ss.str());
                }
            }
        }
    }
}