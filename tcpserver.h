#pragma once

#include <memory>
#include <functional>

#include "iomanager.h"
#include "ipaddress.h"
#include "noncopyable.h"
#include "socket.h"

namespace Framework {
    class TcpServer : public std::enable_shared_from_this<TcpServer>, private Noncopyable {
    public:
        typedef std::shared_ptr<TcpServer> ptr;
        TcpServer(Framework::IOManager* handleClientWorker = Framework::IOManager::GetThis()
                , Framework::IOManager* acceptWorker = Framework::IOManager::GetThis());
        virtual ~TcpServer() {}

        virtual bool bind(Framework::Address::ptr addr);
        virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& failedAddrs);
        virtual bool start(); // 服务器启动
        virtual void stop(); // 服务器关闭

        uint64_t getReadTimeout() const { return m_readTimeout; }
        std::string getName() const { return m_name; }
        void setReadTimeout(uint64_t v) { m_readTimeout = v; }
        void setName(const std::string& v) { m_name = v; }

        bool isStop() const { return m_isStop; }
    protected:
        virtual void handleClient(Socket::ptr client) {}
        virtual void startAccept(Socket::ptr sock);
    private:
        std::vector<Socket::ptr> m_socks; // 可同时listen多个地址
        IOManager* m_acceptWorker; // 只用于建立accept连接的线程池
        IOManager* m_handleClientWorker; // 每accept建立一个连接就把新的连接放到线程池里进行处理
        uint64_t m_readTimeout;
        std::string m_name;
        bool m_isStop;
    };

}