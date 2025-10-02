#include <dlfcn.h> // dlsym

#include "config.h"
#include "fdmanager.h"
#include "hook.h"
#include "iomanager.h"
#include "log.h"

static Framework::Logger::ptr g_logger = LOG_NAME("system");

namespace Framework {
    static Framework::ConfigVar<int>::ptr g_tcp_connect_timeout = Framework::Config::Lookup("tcp.connect.timeout", (int)5000, "tcp connect timeout");
    static thread_local bool t_hook_enable = false; // 线程级的hook

    #define HOOK_FUN(XX) \
        XX(sleep) \
        XX(usleep) \
        XX(nanosleep) \
        XX(socket) \
        XX(connect) \
        XX(accept) \
        XX(read) \
        XX(readv) \
        XX(recv) \
        XX(recvfrom) \
        XX(recvmsg) \
        XX(write) \
        XX(writev) \
        XX(send) \
        XX(sendto) \
        XX(sendmsg) \
        XX(close) \
        XX(fcntl) \
        XX(ioctl) \
        XX(getsockopt) \
        XX(setsockopt)

    void hook_init() {
        static bool is_inited = false;
        if (is_inited) {
            return;
        }
        // 使用 dlsym 动态链接库函数，查找函数地址
        // 例如找到sleep，强转成sleep_fun之后赋值给sleep_f
        // 拦截某个函数的调用（如 sleep 或 usleep），并替换成自定义的实现
        // dlsym(RTLD_NEXT, "sleep") 会查找下一个​（即系统库中的）sleep 函数地址，而不是当前模块中的 sleep 函数。
        // 这样，sleep_f 就指向了系统库的 sleep 函数，而不是当前模块的 sleep 函数。
        // 然后，你可以在自己的代码中提供一个自定义的 sleep 函数，并在调用 sleep_f 之前 / 之后插入额外的逻辑（如日志记录、性能统计等）。
        #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
        // 此处被替换后，sleep变成了我的sleep，sleep_f才是原始sleep
        #undef XX
    }

    // 静态全局变量在main前初始化（会调用构造函数），确保先hook
    // __attribute__((constructor))也能实现类似效果
    static uint64_t s_connect_timeout = -1;
    struct _HookIniter {
        _HookIniter() {
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
                LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
        }
    };
    static _HookIniter s_hook_initer;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }

    // 读写IO统一模板
    // 定时器信息结构体
    struct timer_info {
        int cancelled = 0;
    };
    // 可变参数模板
    template<typename OriginFun, typename... Args>
    static ssize_t IOhook(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {
        // Args&& 是一个通用引用，它可以根据传入的实参类型推导出具体是左值引用还是右值引用
        // 未被hook，使用原函数
        if (!Framework::t_hook_enable) {
            return fun(fd, std::forward<Args>(args)...);
            // 在函数模板中将参数以原始的类型（左值或右值）传递给另一个函数，不做任何修改或拷贝。
        }

        // fd不存在，就默认不是socket
        Framework::FdCtx::ptr ctx = Framework::FdMgr::GetInstance()->get(fd);
        if (!ctx) {
            return fun(fd, std::forward<Args>(args)...);
        }

        if (ctx->isClose()) {
            errno = EBADF;
            return -1;
        }

        // 非socket，使用原函数
        if (!ctx->isSocket() || ctx->getUserNonblock()) {
            return fun(fd, std::forward<Args>(args)...);
        }

        uint64_t to = ctx->getTimeout(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info);

    retry:
        // 在此时调用原函数
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        // 被中断，则一直重试
        while (n == -1 && errno == EINTR) {
            n = fun(fd, std::forward<Args>(args)...);
        }
        // 被阻塞，进入调度
        if (n == -1 && errno == EAGAIN) {
            Framework::IOManager* iom = Framework::IOManager::GetThis();
            Framework::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);
            /*
            为什么这里使用weak_ptr？
            我不希望这个 weak_ptr 去延长 timer_info 的生命周期，我仅仅想观察它是否还存在，并在它还存在的情况下安全地访问它。
            可以确保：​只有当外部仍然持有 shared_ptr（即 tinfo 依然有效）时，我们才能通过 lock() 获取到一个有效的 shared_ptr 来访问对象；否则说明对象已经被释放，我们就不应该再操作它。​
            */

            // 如果设置了超时时间，就设置一个条件定时器，到时间自动cancel（立刻执行事件）
            if (to != (uint64_t)-1) {
                timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (Framework::IOManager::Event)(event));
                    }, winfo);
            }
            int c = 0;
            uint64_t now = 0;

            // 添加事件
            int rt = iom->addEvent(fd, (Framework::IOManager::Event)(event));
            if (rt) {
                if (c) {
                    LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                        << fd << ", " << event << ") retry c=" << c
                        << " used=" << (Framework::GetCurrentUS() - now);
                }
                if (timer) {
                    timer->cancel();
                }
                return -1;
            }
            else {
                // 添加事件成功，让出执行权
                Framework::Fiber::YieldToHold();
                // 从其他地方重新获得执行权后，删掉定时器
                if (timer) {
                    timer->cancel();
                }
                if (tinfo->cancelled) {
                    errno = tinfo->cancelled;
                    return -1;
                }

                // 继续无止境读或写
                goto retry;
            }
        }

        return n;
    }
}

extern "C" {
    #define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
    #undef XX

    struct timer_info {
        int cancelled = 0;
    };
    int IOHook_connect(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
        if (!Framework::t_hook_enable) {
            return connect_f(fd, addr, addrlen);
        }
        Framework::FdCtx::ptr ctx = Framework::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose()) {
            errno = EBADF;
            return -1;
        }

        if (!ctx->isSocket()) {
            return connect_f(fd, addr, addrlen);
        }

        if (ctx->getUserNonblock()) {
            return connect_f(fd, addr, addrlen);
        }

        int n = connect_f(fd, addr, addrlen);
        if (n == 0) {
            return 0;
        }
        else if (n != -1 || errno != EINPROGRESS) {
            return n;
        }

        Framework::IOManager* iom = Framework::IOManager::GetThis();
        Framework::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);

        if (timeout_ms != (uint64_t)-1) {
            timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, Framework::IOManager::WRITE);
                }, winfo);
        }

        // 写事件立刻触发
        int rt = iom->addEvent(fd, Framework::IOManager::WRITE);
        if (rt == 0) {
            Framework::Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        else {
            if (timer) {
                timer->cancel();
            }
            LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
        }

        // 可写之后，连接成功，不需要其他操作了
        int error = 0;
        socklen_t len = sizeof(int);
        if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
            return -1;
        }
        if (!error) {
            return 0;
        }
        else {
            errno = error;
            return -1;
        }
    }

    unsigned int sleep(unsigned int seconds) {
        if (!Framework::t_hook_enable) {
            return sleep_f(seconds);
        }
        Framework::Fiber::ptr fiber = Framework::Fiber::GetThis();
        Framework::IOManager* iom = Framework::IOManager::GetThis();
        // iom->addTimer(seconds * 1000, [iom, fiber]() {iom->schedule(fiber); });
        iom->addTimer(seconds * 1000, std::bind((void(Framework::Scheduler::*)(Framework::Fiber::ptr, int thread))&Framework::IOManager::schedule, iom, fiber, -1));
        Framework::Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec) {
        if (!Framework::t_hook_enable) {
            return usleep_f(usec);
        }
        Framework::Fiber::ptr fiber = Framework::Fiber::GetThis();
        Framework::IOManager* iom = Framework::IOManager::GetThis();
        // iom->addTimer(usec / 1000, [iom, fiber]() {iom->schedule(fiber); });
        iom->addTimer(usec / 1000, std::bind((void(Framework::Scheduler::*)(Framework::Fiber::ptr, int thread))&Framework::IOManager::schedule, iom, fiber, -1));
        Framework::Fiber::YieldToHold();
        return 0;
    }

    int nanosleep(const struct timespec* req, struct timespec* rem) {
        if (!Framework::t_hook_enable) {
            return nanosleep_f(req, rem);
        }
        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        Framework::Fiber::ptr fiber = Framework::Fiber::GetThis();
        Framework::IOManager* iom = Framework::IOManager::GetThis();
        iom->addTimer(timeout_ms, [iom, fiber]() {iom->schedule(fiber); });
        Framework::Fiber::YieldToHold();
        return 0;
    }

    // socket IO
    int socket(int domain, int type, int protocol) {
        if (!Framework::t_hook_enable) {
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        if (fd == -1) {
            return fd;
        }
        Framework::FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    // connect的超时跟一般IO不同
    int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
        //return connect_f(sockfd, addr, addrlen);
        return IOHook_connect(sockfd, addr, addrlen, Framework::s_connect_timeout);
    }

    int accept(int s, struct sockaddr* addr, socklen_t* addrlen) {
        int fd = Framework::IOhook(s, accept_f, "accept", Framework::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0) {
            Framework::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    ssize_t read(int fd, void* buf, size_t count) {
        return Framework::IOhook(fd, read_f, "read", Framework::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec* iov, int iovcnt) {
        return Framework::IOhook(fd, readv_f, "readv", Framework::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
        return Framework::IOhook(sockfd, recv_f, "recv", Framework::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) {
        return Framework::IOhook(sockfd, recvfrom_f, "recvfrom", Framework::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags) {
        return Framework::IOhook(sockfd, recvmsg_f, "recvmsg", Framework::IOManager::READ, SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void* buf, size_t count) {
        return Framework::IOhook(fd, write_f, "write", Framework::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
        return Framework::IOhook(fd, writev_f, "writev", Framework::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int s, const void* msg, size_t len, int flags) {
        return Framework::IOhook(s, send_f, "send", Framework::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
    }

    ssize_t sendto(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen) {
        return Framework::IOhook(s, sendto_f, "sendto", Framework::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
    }

    ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {
        return Framework::IOhook(s, sendmsg_f, "sendmsg", Framework::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
    }

    int close(int fd) {
        if (!Framework::t_hook_enable) {
            return close_f(fd);
        }

        Framework::FdCtx::ptr ctx = Framework::FdMgr::GetInstance()->get(fd);
        if (ctx) {
            auto iom = Framework::IOManager::GetThis();
            if (iom) {
                iom->cancelAll(fd);
            }
            ctx->close();
            Framework::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }

    // 获取UserNonblock信息
    int fcntl(int fd, int cmd, ...) {
        va_list va;
        va_start(va, cmd);
        switch (cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                Framework::FdCtx::ptr ctx = Framework::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                }
                else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                Framework::FdCtx::ptr ctx = Framework::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if (ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                }
                else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:       
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFD:     
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;

        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
        }
    }

    int ioctl(int d, unsigned long int request, ...) {
        va_list va;
        va_start(va, request);
        void* arg = va_arg(va, void*);
        va_end(va);

        if (FIONBIO == request) {
            bool user_nonblock = !!*(int*)arg; // 两次逻辑非操作，用于将任意整数值强制转换为严格的布尔值
            Framework::FdCtx::ptr ctx = Framework::FdMgr::GetInstance()->get(d);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(d, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
        if (!Framework::t_hook_enable) {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        if (level == SOL_SOCKET) {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
                Framework::FdCtx::ptr ctx = Framework::FdMgr::GetInstance()->get(sockfd);
                if (ctx) {
                    const timeval* tv = (const timeval*)optval;
                    ctx->setTimeout(optname, static_cast<uint64_t>(tv->tv_sec) * 1000 + tv->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}  