#pragma once
// �̳߳غ�Э�̵�����
#include <memory>
#include <functional>
#include "fiber.h"
#include "multithread.h"

namespace Framework {
    class Scheduler {
    public:
        typedef std::shared_ptr<Scheduler> ptr;

        Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = ""); // use_caller��ʾ�Ƿ񴴽����������̱߳����Ƿ���ܵ���
        virtual ~Scheduler();

        const std::string& getName() const { return m_name; }

        static Scheduler* GetThis();     
        static Fiber* GetMainFiber(); // ������Ҳ��һ����Э��

        void setThis();
        void start();
        void stop();

        template<class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1) {
            bool need_tickle = false;
            {
                Mutex::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }

            if (need_tickle) {
                tickle();
            }

        }

        // �������ȣ�ȷ��һ������˳��ִ��
        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end) {
                    need_tickle = scheduleNoLock(&*begin) || need_tickle; // ˼��������ָ��ָ����ȡ��ַ��Ϊʲô��
                    ++begin;
                }
            }
            if (need_tickle) {
                tickle();
            }
        }
    protected:
        virtual void tickle();
        void run();
        virtual bool stopping();
        virtual void idle(); // ����Э�̸�ִ�е������ֿա�˯�ߣ�

        // ���ڴ洢�߳�ID������
        std::vector<int> m_threadIds;
        // �̵߳�����
        size_t m_threadCount = 0;
        // ��ǰ��Ծ���߳�����
        std::atomic<size_t> m_activeThreadCount = { 0 };
        // ��ǰ���ڿ���״̬���߳�����
        std::atomic<size_t> m_idleThreadCount = { 0 }; // ԭ������++ --���ü���
        // ��ʶ�Ƿ�����ֹͣ�ı�־��true��ʾ����ֹͣ
        bool m_stopping = true;
        // ��ʶ�Ƿ��Զ�ֹͣ�ı�־��true��ʾ�Զ�ֹͣ
        bool m_autoStop = false;
    private:
        // ��һ��Э�̣���ص����������뵽����������������У������ǰ���ȶ��� m_fibers �ǿյģ���ô�¼�����������Ҫ�����ѡ�������
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread) {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb) {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

        struct FiberAndThread {
            Fiber::ptr fiber;
            std::function<void()> cb;
            int thread;

            FiberAndThread(Fiber::ptr f, int thr)
                :fiber(f), thread(thr) {
            }

            FiberAndThread(Fiber::ptr* f, int thr)
                :thread(thr) {
                fiber.swap(*f);
            }

            FiberAndThread(std::function<void()> f, int thr)
                :cb(f), thread(thr) {
            }

            FiberAndThread(std::function<void()>* f, int thr)
                :thread(thr) {
                cb.swap(*f);
            }

            FiberAndThread() 
                :thread(-1) {
                // Ϊʲô��ҪĬ�Ϲ����������Ϊ����Ҫ�����ŵ�STL��list���Ȼû������ʼ����
            }

            void reset() {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }

        }; // ˼����Ϊʲô���������汾������ָ�롢�Լ�����ָ���ָ�룿����ָ���ָ��Ϊʲô��Ҫʹ��fiber.swap(*f);���������������ƶ����캯����
    private:
        Mutex m_mutex;
        std::vector<Multithread::ptr> m_threads;
        std::list<FiberAndThread> m_fibers; // ��ִ�еĶ��У�ִ����ȿ������̣߳�Ҳ������Э��
        std::string m_name;
        uint32_t m_rootThread;
        Fiber::ptr m_rootFiber;
    };
}