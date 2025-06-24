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
                Scheduler* scheduler = nullptr;     //�¼�����scheduler
                Fiber::ptr fiber;                   //�¼�Э��
                std::function<void()> cb;           //�¼��Ļص�����
            };

            EventContext read;     //���¼�
            EventContext write;    //д�¼�
            int fd = 0;                //�¼������ľ��
            Event m_events = NONE; //�Ѿ�ע����¼�
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

        void contextResize(size_t size); // �����¼����д�С
        bool hasIdleThreads() { return m_idleThreadCount > 0; }
    private:
        int m_epfd = 0;
        int m_tickleFds[2]; // ֪ͨ��pipe�ܵ�

        std::atomic<size_t> m_pendingEventCount = { 0 }; // �ȴ�ִ�е��¼�����
        RWMutex m_mutex; // IO�������Ķ�д��
        std::vector<FdContext*> m_fdContexts; // �¼�����
    };


}