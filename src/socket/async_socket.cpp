#include "async_socket.h"
#include "fdmanager.h"
#include "iomanager.h"
#include "log.h"
#include "macro.h"

// socket new
namespace Framework {
    static Logger::ptr g_logger = LOG_NAME("system");

    Socket::Socket(int family, int type, int protocol)
        : m_sock(-1)
        , m_family(family)
        , m_type(type)
        , m_protocol(protocol)
        , m_isConnected(false) {
    }

    Socket::~Socket() {
        close();
    }

    bool Socket::init(int sock) {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
        if (ctx && ctx->isSocket() && !ctx->isClose()) {
            m_sock = sock;
            m_isConnected = true;
            initSock();
            getLocalAddress();
            getRemoteAddress();
            return true;
        }
        return false;
    }

    void Socket::initSock() {
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        if (m_type == SOCK_STREAM) { // TCP
            setOption(IPPROTO_TCP, TCP_NODELAY, val); // 取消Nagle算法的将多个小数据组成一个大数据再发，Nagle会使得延迟变高
        }
    }

    void Socket::newSock() {
        m_sock = socket(m_family, m_type, m_protocol);
        //if (LIKELY(m_sock != -1)) {
        //    initSock();
        //}
        //else {
        //    LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", " << m_protocol << ") errno=" << errno << " errstr=" << strerror(errno);
        //}
        if (m_sock != -1) [[likely]] {
            initSock();
        }
        else [[unlikely]] {
            LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", " << m_protocol << ") errno=" << errno << " errstr=" << strerror(errno);
        }
    }
}

// socket self info
namespace Framework {
    int64_t Socket::getSendTimeout() {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
        if (ctx) {
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }

    void Socket::setSendTimeout(int64_t v) {
        struct timeval tv { int(v / 1000), int(v % 1000 * 1000) };
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    int64_t Socket::getRecvTimeout() {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
        if (ctx) {
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }

    void Socket::setRecvTimeout(int64_t v) {
        struct timeval tv { int(v / 1000), int(v % 1000 * 1000) };
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
        int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
        if (rt) {
            LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
                << " level=" << level << " option=" << option
                << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
        if (setsockopt(m_sock, level, option, result, (socklen_t)len)) {
            LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
                << " level=" << level << " option=" << option
                << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    Address::ptr Socket::getRemoteAddress() {
        if (m_remoteAddress) {
            return m_remoteAddress;
        }

        Address::ptr result;
        switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
        }
        socklen_t addrlen = result->getAddrLen();
        if (getpeername(m_sock, result->getAddr(), &addrlen)) {
            LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
                << " errono=" << errno << " errstr=" << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family));
        }
        if (m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    Address::ptr Socket::getLocalAddress() {
        if (m_localAddress) {
            return m_localAddress;
        }

        Address::ptr result;
        switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
        }
        socklen_t addrlen = result->getAddrLen();
        if (getsockname(m_sock, result->getAddr(), &addrlen)) {
            LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
                << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family));
        }
        if (m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    bool Socket::isValid() const {
        return m_sock != -1;
    }

    int Socket::getError() {
        int error = 0;
        socklen_t len = sizeof(error);
        if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
            error = errno;
        }
        return error;
    }

    std::ostream& Socket::dump(std::ostream& os) const {
        os << "[Socket sock=" << m_sock
            << " is_connected=" << m_isConnected
            << " family=" << m_family
            << " type=" << m_type
            << " protocol=" << m_protocol;
        if (m_localAddress) {
            os << " local_address=" << m_localAddress->toString();
        }
        if (m_remoteAddress) {
            os << " remote_address=" << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    std::string Socket::toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    bool Socket::cancelRead() {
        return IOManager::GetThis()->cancelEvent(m_sock, Framework::IOManager::READ);
    }

    bool Socket::cancelWrite() {
        return IOManager::GetThis()->cancelEvent(m_sock, Framework::IOManager::WRITE);
    }

    bool Socket::cancelAccept() {
        return IOManager::GetThis()->cancelEvent(m_sock, Framework::IOManager::READ);
    }

    bool Socket::cancelAll() {
        return IOManager::GetThis()->cancelAll(m_sock);
    }
}

// socket API
namespace Framework {
    bool Socket::bind(const Address::ptr addr) {
        if (UNLIKELY(!isValid())) { // UNLIKELY：生成socket出错是小概率事件
            newSock();
            if (UNLIKELY(!isValid())) {
                return false;
            }
        }

        if (UNLIKELY(addr->getFamily() != m_family)) {
            LOG_ERROR(g_logger) << "bind sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }

        if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
            LOG_ERROR(g_logger) << "bind error errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        getLocalAddress(); // 服务器端没有remote地址，只需本地地址
        return true;
    }

    bool Socket::listen(int backlog) {
        if (!isValid()) {
            LOG_ERROR(g_logger) << "listen error sock=-1";
            return false;
        }
        if (::listen(m_sock, backlog)) {
            LOG_ERROR(g_logger) << "listen error errno=" << errno
                << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    Socket::ptr Socket::accept() {
        Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
        int newsock = ::accept(m_sock, nullptr, nullptr); // 非本accept，而是之前hook的那个accept
        if (newsock == -1) {
            LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }
        if (sock->init(newsock)) {
            return sock;
        }
        return nullptr;
    }
    
    int Socket::send(const void* buffer, size_t length, int flags) {
        if (isConnected()) {
            return ::send(m_sock, buffer, length, flags);
        }
        return -1;
    }

    int Socket::send(const iovec* buffers, size_t length, int flags) {
        if (isConnected()) {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }

	int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
		msghdr msg;
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = (iovec*)buffers;
		msg.msg_iovlen = length;
		msg.msg_name = to->getAddr();
		msg.msg_namelen = to->getAddrLen();
		return ::sendmsg(m_sock, &msg, flags);
	}

    int Socket::recv(void* buffer, size_t length, int flags) {
        if (isConnected()) {
            return ::recv(m_sock, buffer, length, flags);
        }
        return -1;
    }

    int Socket::recv(iovec* buffers, size_t length, int flags) {
        if (isConnected()) {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));           
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
        if (isConnected()) {
            socklen_t len = from->getAddrLen();
            return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
        }
        return -1;
    }

    int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
        if (isConnected()) {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = from->getAddr();
            msg.msg_namelen = from->getAddrLen();
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
        if (!isValid()) {
            newSock();
            if (UNLIKELY(!isValid())) {
                return false;
            }
        }

        if (UNLIKELY(addr->getFamily() != m_family)) {
            LOG_ERROR(g_logger) << "connect_sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }

        if (timeout_ms == (uint64_t)-1) {
            if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
                LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") error errno=" << errno << " errstr=" << strerror(errno);
                this->close();
                return false;
            }
        }
        else {
            if (::IOHook_connect(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
                LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") timeout=" << timeout_ms << " error errno="
                    << errno << " errstr=" << strerror(errno);
                this->close();
                return false;
            }
        }
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;
    }

    bool Socket::close() {
        if (!m_isConnected && m_sock == -1) {
            return true;
        }
        m_isConnected = false;
        if (m_sock != -1) {
            ::close(m_sock);
            m_sock = -1;
        }
        return false;
    }
}

// static
namespace Framework {
    Socket::ptr Socket::CreateTCP(Framework::Address::ptr address) {
        Socket::ptr sock(new Socket(address->getFamily(), SOCK_STREAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP(Framework::Address::ptr address) {
        Socket::ptr sock(new Socket(address->getFamily(), SOCK_DGRAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket() {
        Socket::ptr sock(new Socket(AF_INET, SOCK_STREAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket() {
        Socket::ptr sock(new Socket(AF_INET, SOCK_DGRAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixStreamSocket() {
        Socket::ptr sock(new Socket(AF_UNIX, SOCK_STREAM, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixDgramSocket() {
        Socket::ptr sock(new Socket(AF_UNIX, SOCK_DGRAM, 0));
        return sock;
    }
}