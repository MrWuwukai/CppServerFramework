#include "bytearray.h"
#include "iomanager.h"
#include "ipaddress.h"
#include "log.h"
#include "tcpserver.h"

static Framework::Logger::ptr g_logger = LOG_ROOT();

class EchoServer : public Framework::TcpServer {
public:
    EchoServer(int type);
    void handleClient(Framework::Socket::ptr client);

private:
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    :m_type(type) {
}

void EchoServer::handleClient(Framework::Socket::ptr client) {
    LOG_INFO(g_logger) << "handleClient " << client->toString();
    Framework::ByteArray::ptr ba(new Framework::ByteArray); // 序列化数据
    while (true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if (rt == 0) {
            LOG_INFO(g_logger) << "client close: " << client->toString();
            break;
        }
        else if (rt < 0) {
            LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if (m_type == 1) { // text
            LOG_INFO(g_logger) << ba->toString();
        }
        else { // binary data
            LOG_INFO(g_logger) << ba->toHexString();
        }
    }
}

int type = 1; // 默认text

void run() {
    EchoServer::ptr es(new EchoServer(type));
    auto addr = Framework::Address::LookupAny("0.0.0.0:8888");
    while (!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }
    if (!strcmp(argv[1], "-b")) { // binary data
        type = 2;
    }
    Framework::IOManager iom(2);
    iom.schedule(run);
    return 0;
}