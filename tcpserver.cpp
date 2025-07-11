#include "tcpserver.h"
#include "config.h"
#include "log.h"

namespace Framework {
    static Framework::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
        Framework::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");

    static Framework::Logger::ptr g_logger = LOG_NAME("system");

    TcpServer::TcpServer(Framework::IOManager* handleClientWorker, Framework::IOManager* acceptWorker)
        : m_acceptWorker(acceptWorker),
        m_handleClientWorker(handleClientWorker),
        m_readTimeout(g_tcp_server_read_timeout->getValue()),
        m_name("MyServer/1.0.0"),
        m_isStop(false) {
    }

    TcpServer::~TcpServer() {
        for (auto& i : m_socks) {
            i->close();
        }
        m_socks.clear();
    }

    bool TcpServer::bind(Framework::Address::ptr addr) {
        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> failedAddrs;
        addrs.push_back(addr);
        return bind(addrs, failedAddrs);
    }

    bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& failedAddrs) {
        // bind + listen
        for (auto& addr : addrs) {
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock->bind(addr)) {
                LOG_ERROR(g_logger) << "bind fail errno=" << errno << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
                failedAddrs.push_back(addr);
                continue;
            }
            if (!sock->listen()) {
                LOG_ERROR(g_logger) << "listen fail errno=" << errno << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
                failedAddrs.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);
        }
        if (!failedAddrs.empty()) {
            m_socks.clear();
            return false;
        }
        for (auto& i : m_socks) {
            LOG_INFO(g_logger) << "server bind success: " << i->toString();
        }
        return true;
    }

    void TcpServer::startAccept(Socket::ptr sock) {
        while (!m_isStop) {
            Socket::ptr client = sock->accept();
            if (client) {
                client->setRecvTimeout(m_readTimeout);
                // 绑定到自己的智能指针上确保智能指针存活不被释放
                m_handleClientWorker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
            }
            else {
                LOG_ERROR(g_logger) << "accept errno=" << errno << " errstr=" << strerror(errno);
            }
        }
    }

    bool TcpServer::start() {
        if (!m_isStop) {
            return true;
        }
        m_isStop = false;
        for (auto& sock : m_socks) {
            m_acceptWorker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
        }
        return true;
    }

    void TcpServer::stop() {
        m_isStop = true;
        auto self = shared_from_this();
        m_acceptWorker->schedule([this, self] { // 捕获this和self防止突然被析构
            for (auto& sock : m_socks) {
                sock->cancelAll();
                sock->close();
            }
            m_socks.clear();
        });
    }
}