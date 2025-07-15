#include <iostream>
#include "connection.h"
#include "iomanager.h"
#include "log.h"

static Framework::Logger::ptr g_logger = LOG_ROOT();

void run() {
    Framework::Address::ptr addr = Framework::Address::LookupAnyIPAddress("www.baidu.com:80");
    if (!addr) {
        LOG_INFO(g_logger) << "get addr error";
        return;
    }

    Framework::Socket::ptr sock = Framework::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if (!rt) {
        LOG_INFO(g_logger) << "connect " << addr->toString() << " failed";
        return;
    }

    Framework::HTTP::HttpConnection::ptr conn(new Framework::HTTP::HttpConnection(sock));
    Framework::HTTP::HttpRequest::ptr req(new Framework::HTTP::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    LOG_INFO(g_logger) << "req:" << std::endl << req->toString();

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp) {
        LOG_INFO(g_logger) << "recv response error";
        return;
    }
    LOG_INFO(g_logger) << "rsp:" << std::endl << rsp->toString();
}

int main(int argc, char** argv) {
    Framework::IOManager iom(2);
    iom.schedule(run);
    return 0;
}