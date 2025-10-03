#include <sstream>
#include <ifaddrs.h>

#include "ipaddress.h"
#include "endians.h"
#include "log.h"

namespace Framework {
    static Logger::ptr g_logger = LOG_NAME("system");

    template<class T>
    static T CreateMask(uint32_t bits) { // 掩码位数
        return (~(~static_cast<T>(0) << bits));
    }

    inline static bool IsWithinBounds(const char* ptr, const char* start, const char* end) { // 辅助函数：检查 ptr 是否在 [start, end) 范围内
        return ptr >= start && ptr < end;
    }

    template<class T>
    static uint32_t CountBytes(T value) { // 最快速获取一个二进制数里面的1的数量
        uint32_t result = 0;
        for (; value; ++result) {
            value &= value - 1; // 使得value里一定减少一个“1”
        }
        return result;
    }
}

// Address
namespace Framework {
    int Address::getFamily() const {
        return getAddr()->sa_family;
    }

    std::string Address::toString() const {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    bool Address::operator<(const Address& rhs) const {
        socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
        int result = memcmp(getAddr(), rhs.getAddr(), minlen);
        /*
        ​0：两块内存的前 num 字节内容完全相同。
​        < 0​：ptr1 的前 num 字节内容“小于” ptr2 的对应字节（按字节逐个比较，按无符号字符的 ASCII 值判断大小）。
        ​> 0：ptr1 的前 num 字节内容“大于” ptr2 的对应字节。
        */
        if (result < 0) {
            return true;
        }
        else if (result > 0) {
            return false;
        }
        else if (getAddrLen() < rhs.getAddrLen()) {
            return true;
        }
        return false;
    }

    bool Address::operator==(const Address& rhs) const {
        return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
    }

    bool Address::operator!=(const Address& rhs) const {
        return !(*this == rhs);
    }

    Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
        if (addr == nullptr) {
            return nullptr;
        }

        Address::ptr result;
        switch (addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
        }
        return result;
    }
   
    // 域名获取地址
    bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host, int family, int type, int protocol) {
        addrinfo hints, *results, *next;
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        std::string node;
        const char* service = NULL;

        // ipv6地址service检查
        if (!host.empty() && host[0] == '[') {
            const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
            /*
            函数原型：
            void* memchr(const void* ptr, int c, size_t count);
            参数说明：
            ptr​：指向要搜索的内存块的起始地址（const void* 类型，表示可以接受任意类型的数据指针）。
            c：要搜索的字符（以 int 形式传递，但实际会被转换为 unsigned char）。
            count：要搜索的字节数（即内存块的大小）。
            */
            if (endipv6) {
                //check out of range
                if (*(endipv6 + 1) == ':') {
                    if (IsWithinBounds(endipv6 + 2, host.c_str(), host.c_str() + host.size())) {
                        service = endipv6 + 2;
                    }
                    else {
                        // 越界，非法格式
                        LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", " << family << ", " << type << ") err=" << errno << " errstr=" << strerror(errno);
                        return false;
                    }
                }
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }

        // node service检查
        if (node.empty()) {
            service = (const char*)memchr(host.c_str(), ':', host.size());
            if (service) {
                //TODO check out of range
                if (IsWithinBounds(service + 1, host.c_str(), host.c_str() + host.size())) {
                    if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                        node = host.substr(0, service - host.c_str());
                        ++service;
                    }
                }
                else {
                    LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", " << family << ", " << type << ") err=" << errno << " errstr=" << strerror(errno);
                    return false;
                }
            }
        }

        if (node.empty()) {
            node = host;
        }
        int error = getaddrinfo(node.c_str(), service, &hints, &results);
        if (error) {
            LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", " << family << ", " << type << ") err=" << error << " errstr=" << strerror(errno);
            return false;
        }

        next = results;
        while (next) {
            result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
            next = next->ai_next;
        }
        freeaddrinfo(results);
        return true;
    }

    Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol) {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol)) {
            return result[0];
        }
        return nullptr;
    }

    std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol) {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol)) {
            for (auto& i : result) {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
                if (v) {
                    return v;
                }
            }
        }
        return nullptr;
    }

    // 服务器网卡信息转地址（未指定网卡名字）
    bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family) {
        struct ifaddrs *next, *results;
        if (getifaddrs(&results) != 0) {
            LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs"
                                << " err=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        
        try {
            for (next = results; next; next = next->ifa_next) {
                Address::ptr addr;
                uint32_t prefix_length = ~0u;
                if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                    continue;
                }
                switch (next->ifa_addr->sa_family) {
                case AF_INET: 
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_length = CountBytes(netmask);
                }
                    break;
                case AF_INET6: 
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                    prefix_length = 0;
                    for (int i = 0; i < 16; ++i) {
                        prefix_length += CountBytes(netmask.s6_addr[i]);
                    }
                }
                    break;
                default:
                    break;
                }

                if (addr) {
                    result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_length)));
                }
            }
        }
        catch (...) {
            LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception"
                                << " err=" << errno << " errstr=" << strerror(errno);
            freeifaddrs(results);
            return false;
        }

        freeifaddrs(results);
        return true;
    }

    // 服务器网卡信息转地址（指定网卡名字）
    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family) {
        if (iface.empty() || iface == "*") {
            if (family == AF_INET || family == AF_UNSPEC) {
                // result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
            }
            if (family == AF_INET6 || family == AF_UNSPEC) {
                result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
            }
            return true;
        }

        std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;

        if (!GetInterfaceAddresses(results, family)) {
            return false;
        }

        auto its = results.equal_range(iface); // multimap的特有查找方法，查找iface，返回两个迭代器表示范围
        for (; its.first != its.second; ++its.first) {
            result.push_back(its.first->second);
        }
        return true;
    }
}

// IPAddress
namespace Framework {

    // 从地址字符串创造地址，管你是v4还是v6还是域名
    IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
        addrinfo hints, *results;
        memset(&hints, 0, sizeof(addrinfo));  // 初始化hints结构体，将内存置为0

        hints.ai_flags = AI_NUMERICHOST;  // 设置提示标志，表示使用数字形式的主机地址
        hints.ai_family = AF_UNSPEC;  // 设置地址族为未指定，可以是IPv4或IPv6

        int error = getaddrinfo(address, NULL, &hints, &results);  // 获取地址信息
        if (error) {
            LOG_ERROR(g_logger) << "IPAddress::Create(" << address << ", " << port << ") error=" << error
                                << " errno=" << errno << " errstr=" << strerror(errno);  // 记录错误日志
            return nullptr;
        }

        try {
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));  // 动态转换为IPAddress类型的智能指针
                
            if (result) {
                result->setPort(port);  // 设置端口号
            }
            freeaddrinfo(results);  // 释放getaddrinfo分配的内存
            return result;
        }
        catch (...) {
            freeaddrinfo(results);  // 发生异常时释放内存
            return nullptr;
        }
    }

    // IPv4Address::IPv4Address() {
    //     memset(&m_addr, 0, sizeof(m_addr));
    //     m_addr.sin_family = AF_INET;

    // }
    IPv4Address::IPv4Address(const sockaddr_in& address) {
        m_addr = address;
    }
    /*
    这段代码不是直接使用标准库中的htons、htonl等函数。几个原因：
    ​跨平台一致性​：
        标准库函数如htons、htonl等虽然能完成字节序转换，但它们的行为是依赖于平台的(由系统头文件定义)。
        这段代码可能是在一个需要严格控制字节序转换行为的跨平台项目中，开发者可能想要完全控制转换逻辑，而不是依赖不同平台可能存在的实现差异。
    ​大端/小端统一处理​：
        代码中通过THIS_BYTE_ORDER宏判断当前系统的字节序，然后决定是否需要交换字节序。
        这种设计使得代码在大小端系统上都能正确工作，而且逻辑更集中和明确。
    */
    IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = byteswapOnLittleEndian(port);
        m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
    }

    const sockaddr* IPv4Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    sockaddr* IPv4Address::getAddr() {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream& IPv4Address::insert(std::ostream& os) const {
        uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
        os << ((addr >> 24) & 0xff) << "."
            << ((addr >> 16) & 0xff) << "."
            << ((addr >> 8) & 0xff) << "."
            << (addr & 0xff);
        os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr |= byteswapOnLittleEndian(~CreateMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    // 网段
    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    // 子网
    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
        sockaddr_in subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(subnet));
    }

    uint16_t IPv4Address::getPort() const {
        return byteswapOnLittleEndian(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint16_t v) {
        m_addr.sin_port = byteswapOnLittleEndian(v);
    }

    // 从地址字符串创造地址
    IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
        IPv4Address::ptr rt(new IPv4Address);
        rt->m_addr.sin_port = byteswapOnLittleEndian(port);
        int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
        if (result <= 0) {
            LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }

    IPv6Address::IPv6Address() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const sockaddr_in6& address) {
        m_addr = address;
    }

    IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteswapOnLittleEndian(port);
        memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
    }

    const sockaddr* IPv6Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    sockaddr* IPv6Address::getAddr() {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const {
        return sizeof(m_addr);

    }
    std::ostream& IPv6Address::insert(std::ostream& os) const {
        os << "[";
        uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
        bool used_zeros = false;
        for (size_t i = 0; i < 8; ++i) {
            if (addr[i] == 0 && !used_zeros) {
                continue;
            }
            if (i && addr[i - 1] == 0 && !used_zeros) {
                os << ":";
                used_zeros = true;
            }
            if (i) {
                os << ":";
            }
            os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
        }
        if (!used_zeros && addr[7] == 0) {
            os << "::";
        }
        os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |= ~CreateMask<uint8_t>(prefix_len % 8);
        for (int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefix_len / 8] = CreateMask<uint8_t>(prefix_len % 8);
        for (uint32_t i = 0; i < prefix_len / 8; ++i) {
            subnet.sin6_addr.s6_addr[i] = 0xFF;
        }
        return IPv6Address::ptr(new IPv6Address(subnet));
    }

    uint16_t IPv6Address::getPort() const {
        return byteswapOnLittleEndian(m_addr.sin6_port);
    }

    void IPv6Address::setPort(uint16_t v) {
        m_addr.sin6_port = byteswapOnLittleEndian(v);
    }

    // 从地址字符串创造地址
    IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
        IPv6Address::ptr rt(new IPv6Address);
        rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
        int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
        if (result < 0) {
            LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }
}

// UnixAddress
namespace Framework {
    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

    UnixAddress::UnixAddress() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string& path) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size() + 1;

        if (!path.empty() && path[0] == '\0') {
            --m_length;
        }

        if (m_length > sizeof(m_addr.sun_path)) {
            throw std::logic_error("path too long");
        }
        memcpy(m_addr.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);
    }

    const sockaddr* UnixAddress::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    sockaddr* UnixAddress::getAddr() {
        return (sockaddr*)&m_addr;
    }

    socklen_t UnixAddress::getAddrLen() const {
        return m_length;
    }

    void UnixAddress::setAddrLen(uint32_t v) {
        m_length = v;
    }

    std::ostream& UnixAddress::insert(std::ostream& os) const {
        if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
            return os << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        return os << m_addr.sun_path;
    }
}

// UnknownAddress
namespace Framework {
    UnknownAddress::UnknownAddress(int family) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;
    }

    UnknownAddress::UnknownAddress(const sockaddr& addr) {
        m_addr = addr;
    }

    const sockaddr* UnknownAddress::getAddr() const {
        return &m_addr;
    }

    sockaddr* UnknownAddress::getAddr() {
        return &m_addr;
    }

    socklen_t UnknownAddress::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream& UnknownAddress::insert(std::ostream& os) const {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Address& addr) {
        return addr.insert(os);
    }
}