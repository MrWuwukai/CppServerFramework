#pragma once
// 线程池和协程调度器
#include <memory>
#include <functional>
#include "fiber.h"
#include "multithread.h"

namespace Framework {
    class Scheduler {
    public:
        typedef std::shared_ptr<Scheduler> ptr;

        Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = ""); // use_caller表示是否创建调度器的线程本身是否接受调度
        virtual ~Scheduler();

        const std::string& getName() const { return m_name; }

        static Scheduler* GetThis();     
        static Fiber* GetMainFiber(); // 调度器也有一个主协程

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

        // 批量调度，确保一组任务顺序执行
        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end) {
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle; // 思考：这里指针指出又取地址是为什么？
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
        virtual void idle(); // 空闲协程该执行的任务，轮空、睡眠？

        // 用于存储线程ID的向量
        std::vector<int> m_threadIds;
        // 线程的总数
        size_t m_threadCount = 0;
        // 当前活跃的线程数量
        std::atomic<size_t> m_activeThreadCount = { 0 };
        // 当前处于空闲状态的线程数量
        std::atomic<size_t> m_idleThreadCount = { 0 }; // 原子量，++ --不用加锁
        // 标识是否正在停止的标志，true表示正在停止
        bool m_stopping = true;
        // 标识是否自动停止的标志，true表示自动停止
        bool m_autoStop = false;
    private:
        // 将一个协程（或回调函数）加入到调度器的任务队列中，如果当前调度队列 m_fibers 是空的，那么新加入的任务就需要“唤醒”调度器
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
                // 为什么需要默认构造参数？因为等下要把他放到STL的list里，不然没法做初始化。
            }

            void reset() {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }

        }; // 思考：为什么会有两个版本：智能指针、以及智能指针的指针？智能指针的指针为什么需要使用fiber.swap(*f);？这里可以替代成移动构造函数吗？
    private:
        Mutex m_mutex;
        std::vector<Multithread::ptr> m_threads;
        std::list<FiberAndThread> m_fibers; // 待执行的队列，执行体既可以是线程，也可以是协程
        std::string m_name;
        uint32_t m_rootThread;
        Fiber::ptr m_rootFiber;
    };
}