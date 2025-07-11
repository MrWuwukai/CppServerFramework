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
        virtual bool start(); // ����������
        virtual void stop(); // �������ر�

        uint64_t getReadTimeout() const { return m_readTimeout; }
        std::string getName() const { return m_name; }
        void setReadTimeout(uint64_t v) { m_readTimeout = v; }
        void setName(const std::string& v) { m_name = v; }

        bool isStop() const { return m_isStop; }
    protected:
        virtual void handleClient(Socket::ptr client) {}
        virtual void startAccept(Socket::ptr sock);
    private:
        std::vector<Socket::ptr> m_socks; // ��ͬʱlisten�����ַ
        IOManager* m_acceptWorker; // ֻ���ڽ���accept���ӵ��̳߳�
        IOManager* m_handleClientWorker; // ÿaccept����һ�����ӾͰ��µ����ӷŵ��̳߳�����д���
        uint64_t m_readTimeout;
        std::string m_name;
        bool m_isStop;
    };

}