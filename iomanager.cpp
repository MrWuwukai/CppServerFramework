#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace Framework {
	static Framework::Logger::ptr g_logger = LOG_NAME("system");

    IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
        : Scheduler(threads, use_caller, name) {
        // 创建一个epoll实例，最大文件描述符数量设置为5000
        m_epfd = epoll_create(5000);
        // 断言epoll实例创建成功（m_epfd大于0）
        ASSERT(m_epfd > 0);

        // 创建一个管道，用于线程间通信等用途
        int rt = pipe(m_tickleFds);
        // 断言管道创建成功（返回值为0）
        ASSERT(!rt);

        epoll_event event;
        // 将epoll_event结构体清零
        memset(&event, 0, sizeof(epoll_event));
        // 设置关注的事件类型为EPOLLIN（读就绪）和EPOLLET（边缘触发）
        event.events = EPOLLIN | EPOLLET;
        // 将要监控的文件描述符设置为管道的读端
        event.data.fd = m_tickleFds[0];

        // 将管道的读端设置为非阻塞模式
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        // 断言设置非阻塞模式操作成功（返回值为0）
        ASSERT(!rt);

        // 将管道的读端添加到epoll监控列表中
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        // 断言添加操作成功（返回值为0）
        ASSERT(!rt);

        // 任务队列初始化
        contextResize(32);

        // 启动调度器相关操作
        start();
    }

    IOManager::~IOManager() {
        stop(); // 停止调度器
        close(m_epfd); // 关闭epoll文件描述符
        close(m_tickleFds[0]); // 关闭通知管道
        close(m_tickleFds[1]); // 关闭通知管道
        for (size_t i = 0; i < m_fdContexts.size(); ++i) { // 清空任务队列
            if (m_fdContexts[i]) {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size) {
        // 空间换时间，队列的索引值=文件描述符，减少锁的粒度
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i) {
            if (!m_fdContexts[i]) {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    int IOManager::addEvent(int fd, IOManager::Event event, std::function<void()> cb) {
        // 从事件队列里拿一个（空）事件，拿不到扩容了再拿
        FdContext* fd_ctx = nullptr;
        RWMutex::ReadLock lock(m_mutex);
        if (m_fdContexts.size() > fd) {
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else {
            lock.unlock();
            RWMutex::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        Mutex::Lock lock2(fd_ctx->mutex);
        if (fd_ctx->m_events & event) { // 如果事件已经被注册，则有问题
            LOG_ERROR(g_logger) << "addEvent assert fd=" << fd << " event=" << event << " fd_ctx.event=" << fd_ctx->m_events;
            ASSERT(!(fd_ctx->m_events & event));
        }

        // 把事件修改到/挂到epoll红黑树上，事件=原有事件按位或新事件
        int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->m_events | event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << "," << fd << "," << epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return -1;
        }
        ++m_pendingEventCount;

        // 把事件要做的函数或者协程放到这个事件里，要求在放入前这是一个空事件
        fd_ctx->m_events = (IOManager::Event)(fd_ctx->m_events | event);
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if (cb) {
            event_ctx.cb.swap(cb);
        }
        else {
            event_ctx.fiber = Fiber::GetThis();
            ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event) {
        // 从事件队列里取出事件（准备删了他），如果超过队列长度，则出错
        RWMutex::ReadLock lock(m_mutex);
        if (m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        Mutex::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event)) {
            return false;
        }

        // 从m_events里删一个事件，如果读写都被删了，就是删掉红黑树节点，否则是修改
        Event new_events = (Event)(fd_ctx->m_events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << "," << fd << "," << epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;

        // 清理掉这个事件的cb、协程等信息
        fd_ctx->m_events = new_events;
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event) {
        // cancel和del的不同是，cancel会强制执行事件再删除
        // 从事件队列里取出事件（准备删了他），如果超过队列长度，则出错
        RWMutex::ReadLock lock(m_mutex);
        if (m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        Mutex::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event)) {
            return false;
        }

        // 从m_events里删一个事件，如果读写都被删了，就是删掉红黑树节点，否则是修改
        Event new_events = (Event)(fd_ctx->m_events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events; // 边缘触发
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << "," << fd << "," << epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        // 强制触发事件
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd) {
        // 删除所有事件：读和写
        // 从事件队列里取出事件（准备删了他），如果超过队列长度，则出错
        RWMutex::ReadLock lock(m_mutex);
        if (m_fdContexts.size() <= fd) {
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        Mutex::Lock lock2(fd_ctx->mutex);
        if (!fd_ctx->m_events) {
            return false;
        }

        // 删掉红黑树节点
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt) {
            LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << "," << fd << "," << epevent.events << "):"
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        // 强制触发事件
        if (fd_ctx->m_events & IOManager::Event::READ) {
            fd_ctx->triggerEvent(IOManager::Event::READ);
            --m_pendingEventCount;
        }
        if (fd_ctx->m_events & IOManager::Event::WRITE) {
            fd_ctx->triggerEvent(IOManager::Event::WRITE);
            --m_pendingEventCount;
        }

        ASSERT(fd_ctx->m_events == 0);
        return true;
    }

    IOManager* IOManager::GetThis() {
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }
}

namespace Framework {
    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
        switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            ASSERT_W(false, "getContext");
        }
    }

    void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx) {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event event) {
        ASSERT(m_events & event);
        m_events = (Event)(m_events & ~event);
        EventContext& ctx = getContext(event);
        if (ctx.cb) {
            ctx.scheduler->schedule(&ctx.cb); // 传递地址自动被swap掉，所有权被move了
        }
        else {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr; // 调度器使用完成，置空
    }
}

// 实现父类虚函数
namespace Framework {
    // 向管道写数据，如果有空闲线程或者陷入epoll wait的线程，则唤醒这个线程执行任务
    void IOManager::tickle() {
        if (!hasIdleThreads()) {
            return;
        }
        int rt = write(m_tickleFds[1], "T", 1);
        ASSERT(rt == 1);
    }

    // 满足父类的停止条件，且未解决的事件为0，即可停止
    bool IOManager::stopping() {
        return Scheduler::stopping() && m_pendingEventCount == 0;
    }

    void IOManager::idle() {
        epoll_event* events = new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
            delete[] ptr;
        }); // 智能指针不一定支持数组，必须显式指定自定义删除器​（如 delete[]）

        while (true) {
            if (stopping()) {
                LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
                break;
            }

            int rt = 0;
            do {
                static const int MAX_TIMEOUT = 5000; // epoll支持毫秒级粒度，设置定时器也只需设置成毫秒级即可
                rt = epoll_wait(m_epfd, events, 64, MAX_TIMEOUT); // 等待事件，没有事件超时后自动唤醒

                if (rt < 0 && errno == EINTR) { // 如果没有等待到事件，或只是操作系统的中断事件，则继续等待
                }
                else {
                    break; // 有事件，则退出这层循环
                }
            } while (true);

            // 对于epoll_wait获取的所有事件做操作
            for (int i = 0; i < rt; ++i) {
                epoll_event& event = events[i];
                if (event.data.fd == m_tickleFds[0]) {
                    uint8_t dummy;
                    while (read(m_tickleFds[0], &dummy, 1) == 1);
                    /*在 ET 模式下，如果文件描述符可读，epoll 只会通知一次。
                    如果应用程序没有一次性读取完所有数据，后续即使有更多数据到达，epoll 也不会再次通知，除非文件描述符的状态再次发生变化（比如又有新数据写入）。
                    因此，在 ET 模式下，通常需要循环读取直到 read 返回 EAGAIN 或 EWOULDBLOCK，以确保所有数据都被处理。
                    尽管 tickle fd 通常只写入 1 字节，但也需要while循环保证管道被清空。*/
                    continue; // 如果是管道通知事件，则直接继续
                }

                FdContext* fd_ctx = (FdContext*)event.data.ptr;
                Mutex::Lock lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP)) { // epoll遇到错误或中断，添加读写事件
                    event.events |= EPOLLIN | EPOLLOUT;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN) { // EPOLL返回的事件是读
                    real_events |= IOManager::Event::READ;
                }
                if (event.events & EPOLLOUT) { // EPOLL返回的事件是写
                    real_events |= IOManager::Event::WRITE;
                }

                // 如果epoll红黑树上挂载的事件和epoll wait返回的待执行事件没有交集，例如我挂载读事件，但是epoll返回的是EPOLLOUT写事件，那么就无需执行任何操作
                if ((fd_ctx->m_events & real_events) == IOManager::Event::NONE) {
                    continue;
                }

                // 如果这个epoll wait等来的事件是我需要处理的话，则需要把它从epoll红黑树上修改（如果所有事件都取出则是摘下）
                int left_events = (fd_ctx->m_events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2) {
                    LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                        << op << "," << fd_ctx->fd << "," << event.events << ");"
                        << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                // 最后，有读事件解决读事件，有写事件解决写事件
                if (real_events & IOManager::Event::READ) {
                    fd_ctx->triggerEvent(IOManager::Event::READ);
                    --m_pendingEventCount;
                }
                if (real_events & IOManager::Event::WRITE) {
                    fd_ctx->triggerEvent(IOManager::Event::WRITE);
                    --m_pendingEventCount;
                }
            }

            // 处理完for循环里的所有事件之后，让出执行权到协程调度框架
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();
            raw_ptr->swapOut();
        }
    }
}