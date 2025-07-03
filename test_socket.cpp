#include "socket.h"
#include "log.h"
#include "iomanager.h"

static Framework::Logger::ptr g_looger = LOG_ROOT();

void test_socket() {
    Framework::IPAddress::ptr addr = Framework::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr) {
        LOG_INFO(g_looger) << "get address: " << addr->toString();
    }
    else {
        LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    Framework::Socket::ptr sock = Framework::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr)) {
        LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    }
    else {
        LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if (rt <= 0) {
        LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if (rt <= 0) {
        LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    LOG_INFO(g_looger) << buffs;
}

int main(int argc, char** argv) {
    Framework::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}