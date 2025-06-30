#pragma once
#include <memory>
#include <vector>
#include "iomanager.h"
#include "multithread.h"
#include "singleton.h"

namespace Framework {
    class FdCtx : public std::enable_shared_from_this<FdCtx> {
    public:
        typedef std::shared_ptr<FdCtx> ptr;
        FdCtx(int fd);
        ~FdCtx();

        bool init();
        bool isInit() const { return m_isInit; }
        bool isSocket() const { return m_isSocket; }
        bool isClose() const { return m_isClosed; }
        void close() { m_isClosed = true; }

        void setUserNonblock(bool v) { m_userNonblock = v; }
        bool getUserNonblock() const { return m_userNonblock; }

        void setSysNonblock(bool v) { m_sysNonblock = v; }
        bool getSysNonblock() const { return m_sysNonblock; }

        void setTimeout(int type, uint64_t v);
        uint64_t getTimeout(int type);
    private:
        bool m_isInit : 1;
        bool m_isSocket : 1;
        bool m_sysNonblock : 1; // 系统设置为异步（非阻塞）
        bool m_userNonblock : 1; // 系统设置为异步（非阻塞）
        bool m_isClosed : 1;
        int m_fd;
        uint64_t m_recvTimeout;
        uint64_t m_sendTimeout;
        Framework::IOManager* m_iomanager = nullptr;
    };

    class FdManager {
    public:
        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);
        void del(int fd);

    private:
        RWMutex m_mutex;
        std::vector<FdCtx::ptr> m_datas;
    };

    typedef Singleton<FdManager> FdMgr; // 把FdManager弄成单例模式
}