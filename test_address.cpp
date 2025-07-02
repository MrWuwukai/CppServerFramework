#include "ipaddress.h"
#include "log.h"

Framework::Logger::ptr g_logger = LOG_ROOT();

void test() {
    std::vector<Framework::Address::ptr> addrs;

    bool v = Framework::Address::Lookup(addrs, "www.baidu.com");
    if (!v) {
        LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i) {
        LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<Framework::Address::ptr, uint32_t> > results;
    bool v = Framework::Address::GetInterfaceAddresses(results);
    if (!v) {
        LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }
    for (auto& i : results) {
        LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
    }
}

void test_ipv4() {
    auto addr = Framework::IPAddress::Create("www.baidu.com");
    if (addr) {
        LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test();
    return 0;
}