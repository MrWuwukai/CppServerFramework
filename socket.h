#pragma once

#include <memory>

#include "ipaddress.h" 
#include "noncopyable.h"
#include "hook.h"

namespace Framework {
    class Socket : public std::enable_shared_from_this<Socket>, private Noncopyable {
    public:
        typedef std::shared_ptr<Socket> ptr;
        typedef std::weak_ptr<Socket> weak_ptr;

        Socket(int family, int type, int protocol = 0);
        ~Socket();      

        int64_t getSendTimeout();
        void setSendTimeout(int64_t v);

        int64_t getRecvTimeout();
        void setRecvTimeout(int64_t v);

        bool getOption(int level, int option, void* result, size_t* len);
        template<class T>
        bool getOption(int level, int option, T& result) {
            size_t length = sizeof(T);
            return getOption(level, option, &result, &length);
        }

        bool setOption(int level, int option, const void* result, size_t len);
        template<class T>
        bool setOption(int level, int option, const T& value) {
            return setOption(level, option, &value, sizeof(T));
        }

        // API
        bool bind(const Address::ptr addr);
        bool listen(int backlog = SOMAXCONN);
        Socket::ptr accept();       

        int send(const void* buffer, size_t length, int flags = 0);
        int send(const iovec* buffers, size_t length, int flags = 0); // length = buffers数组里的个数
        int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0); // UDP
        int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0); // UDP

        int recv(void* buffer, size_t length, int flags = 0);
        int recv(iovec* buffers, size_t length, int flags = 0);
        int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0); // UDP
        int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0); // UDP

        bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
        bool close();

        // self info
        Address::ptr getRemoteAddress();
        Address::ptr getLocalAddress();

        int getFamily() const { return m_family; }
        int getType() const { return m_type; }
        int getProtocol() const { return m_protocol; }

        bool isConnected() const { return m_isConnected; }
        bool isValid() const;
        int getError();

        std::ostream& dump(std::ostream& os) const;
        std::string toString() const;
        int getSocket() const { return m_sock; }

        bool cancelRead();
        bool cancelWrite();
        bool cancelAccept();
        bool cancelAll();
    public:
        static Socket::ptr CreateTCP(Framework::Address::ptr address);
        static Socket::ptr CreateUDP(Framework::Address::ptr address);

        static Socket::ptr CreateTCPSocket();
        static Socket::ptr CreateUDPSocket();

        static Socket::ptr CreateUnixStreamSocket();
        static Socket::ptr CreateUnixDgramSocket();
    private:
        void initSock();
        void newSock();
        bool init(int sock);
    private:
        int m_sock;
        int m_family;
        int m_type;
        int m_protocol;
        bool m_isConnected;
        Address::ptr m_localAddress;
        Address::ptr m_remoteAddress;
    };
}