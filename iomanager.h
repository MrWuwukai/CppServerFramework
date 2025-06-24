#pragma once
#include "scheduler.h"

namespace Framework {
    class IOManager : public Scheduler {
    public:
        typedef std::shared_ptr<IOManager> ptr;

        enum Event {
            NONE = 0x0,
            READ = 0x1, // EPOLLIN
            WRITE = 0x4 // EPOLLOUT
        };
    private:
        struct FdContext {
            struct EventContext {
                Scheduler* scheduler = nullptr;     //事件所属scheduler
                Fiber::ptr fiber;                   //事件协程
                std::function<void()> cb;           //事件的回调函数
            };

            EventContext read;     //读事件
            EventContext write;    //写事件
            int fd = 0;                //事件关联的句柄
            Event m_events = NONE; //已经注册的事件
            Mutex mutex;

            EventContext& getContext(Event event);
            void resetContext(EventContext& ctx);
            void triggerEvent(Event event);
        };
    public:
        IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
        ~IOManager();

        //0 success, -1 error
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);

        bool cancelAll(int fd);

        static IOManager* GetThis();
    protected:
        void tickle() override;
        bool stopping() override;
        void idle() override;

        void contextResize(size_t size); // 重置事件队列大小
        bool hasIdleThreads() { return m_idleThreadCount > 0; }
    private:
        int m_epfd = 0;
        int m_tickleFds[2]; // 通知的pipe管道

        std::atomic<size_t> m_pendingEventCount = { 0 }; // 等待执行的事件数量
        RWMutex m_mutex; // IO调度器的读写锁
        std::vector<FdContext*> m_fdContexts; // 事件队列
    };


}