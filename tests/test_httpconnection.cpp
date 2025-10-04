#include <iostream>
#include "http_connection.h"
#include "iomanager.h"
#include "log.h"

static Framework::Logger::ptr g_logger = LOG_ROOT();

void test_pool() {
    Framework::HTTP::HttpConnectionPool::ptr pool(new Framework::HTTP::HttpConnectionPool(
        "www.baidu.com", "", 80, 10, 1000 * 30, 20));

    Framework::IOManager::GetThis()->addTimer(1000, [pool]() {
        auto r = pool->GET("/", 3000);
        LOG_INFO(g_logger) << r->toString();
        }, true);
}

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

    LOG_INFO(g_logger) << "=========================";
    auto rt2 = Framework::HTTP::HttpConnection::GET("http://www.baidu.com/", 300);
    LOG_INFO(g_logger) << "result=" << rt2->result
        << " error=" << rt2->error
        << " rsp=" << (rt2->response ? rt2->response->toString() : "");

    LOG_INFO(g_logger) << "=============================";
    test_pool();
}

int main(int argc, char** argv) {
    Framework::IOManager iom(2);
    iom.schedule(run);
    return 0;
}