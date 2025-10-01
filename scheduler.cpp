#include <vector>
#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace Framework {
	static Logger::ptr g_logger = LOG_NAME("system");

	static thread_local Scheduler* t_scheduler = nullptr; // 当前线程关联的调度器
	static thread_local Fiber* t_fiber = nullptr; // 当前线程正在执行的协程(当前协程)

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
        :m_name(name){
        ASSERT(threads > 0);

        if (use_caller) {
            Fiber::GetThis(); // 本线程也接受调度，则尝试获取、没有则创建创建这个线程的主协程
            --threads; // 本线程也接受调度，无需额外加一个线程

            ASSERT(GetThis() == nullptr); // 确保本线程不能已经拥有一个调度器
            t_scheduler = this; // 当前线程的调度器就是正在构造的这个 Scheduler 对象
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true)); // 设置调度器主协程
            Multithread::SetName(m_name);
            
            t_fiber = m_rootFiber.get(); // 当前线程正在执行的协程就是调度器主协程
            m_rootThread = GetThreadId(); // 设置调度器主线程
            m_threadIds.push_back(m_rootThread);
        }
        else {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler() {
        ASSERT(m_stopping);
        if (GetThis() == this) {
            t_scheduler = nullptr;
        }
    }

    Scheduler* Scheduler::GetThis() {
        return t_scheduler;
    }

    Fiber* Scheduler::GetMainFiber() {
        return t_fiber;
    }

    void Scheduler::setThis() {
        t_scheduler = this;
    }

    void Scheduler::start() {
        // 启动线程池，需要当前未启动且线程池为空
        Mutex::Lock lock(m_mutex);
        if (!m_stopping) {
            return;
        }
        m_stopping = false;
        ASSERT(m_threads.empty());

        // 注册线程到线程池，统一执行run方法
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Multithread(std::bind(&Scheduler::run, this)
                , m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId()); // 线程部信号量控制必能获得id（见线程头文件）
        }
        //lock.unlock();

        /*if (m_rootFiber){
            m_rootFiber->call();
	    }*/
    }

    void Scheduler::stop() {
        // 等所有任务执行完毕后，停止整个线程池
        m_autoStop = true;

        // 若线程池里没有线程，只有一个控制器主协程
        if (m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
            LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if (stopping()) {
                return;
            }
        }

        // 若线程池里有线程，则唤醒线程并让他们自动结束，如果use_caller，则该线程需要特殊处理
        bool exit_on_this_fiber = false;
        if (m_rootThread != -1) {
            ASSERT(GetThis() == this);
        }
        else {
            ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i) {
            tickle();
        }

        if (m_rootFiber) {
            tickle();
        }

        if (m_rootFiber) {
            while (!stopping()) {
                //if (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::EXCEPT) {
                //    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
                //    LOG_INFO(g_logger) << " root fiber is term, reset";
                //    t_fiber = m_rootFiber.get();
                //}
                m_rootFiber->call();
            }
        }

        std::vector<Multithread::ptr> thrs;
        {
            Mutex::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for (auto& i : thrs) {
            i->join();
        }

        //if(exit_on_this_fiber) {
        //}
    }

    void Scheduler::run() {
        set_hook_enable(true);
        // 设置当前调度器为本线程所属调度器
        setThis();
        // 如果当前线程不是主线程，设置一下当前线程的正在执行run的协程
        if (Framework::GetThreadId() != m_rootThread) {
            t_fiber = Fiber::GetThis().get();
        }

        // 创建空闲协程，调度器内无任务、或调度结束之后，就会执行空闲协程，空闲时该干什么取决于具体实现
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while (true) {
            // 重置FiberAndThread对象
            ft.reset();
            // 标记是否有线程需要被唤醒
            bool tickle_me = false;
            bool is_active = false;
            // 从fibers队列里面取出一个任务给ft
            {
                // 锁定互斥锁，保护对m_fibers容器的访问
                Mutex::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end()) {
                    // 如果指定执行的线程不是本线程，则需要唤醒那个线程
                    if (it->thread != -1 && it->thread != Framework::GetThreadId()) {
                        // 移动到下一个任务
                        ++it;
                        // 标记需要唤醒其他线程
                        tickle_me = true;
                        continue;
                    }
                    // 断言：任务要么有纤程，要么有回调函数
                    ASSERT(it->fiber || it->cb);
                    // 如果任务有纤程且纤程处于执行状态，已经执行就不需要执行了
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                        // 移动到下一个任务
                        ++it;
                        continue;
                    }
                    // 将当前任务赋值给ft
                    ft = *it;
                    // 从任务列表中移除该任务
                    m_fibers.erase(it);
                    ++m_activeThreadCount;
                    is_active = true;

                    break;
                }
            }
            // 如果有线程需要被唤醒
            if (tickle_me) {
                // 执行唤醒操作
                tickle();
            }
            // 如果ft中有纤程且纤程状态不是终止状态
            if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {              
                // 将纤程切换进来执行
                ft.fiber->swapIn();
                --m_activeThreadCount;

                // 如果纤程状态变为就绪
                if (ft.fiber->getState() == Fiber::READY) {
                    // 重新扔到队列里
                    schedule(ft.fiber);
                }
                // 如果纤程状态既不是终止状态也不是异常状态
                else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                    // 将纤程状态设置为保持
                    ft.fiber->setState(Fiber::HOLD);
                }
                // 重置ft
                ft.reset();
            }
            // 如果不是Fiber而是普通回调
            else if (ft.cb) {
                if (cb_fiber) {
                    cb_fiber->reset(ft.cb);
                }
                else {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();

                cb_fiber->swapIn();
                --m_activeThreadCount;

                if (cb_fiber->getState() == Fiber::READY) {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
                    cb_fiber->reset(nullptr);
                }
                else {//if(cb_fiber->getState() != Fiber::TERM) {
                    cb_fiber->setState(Fiber::HOLD);
                    cb_fiber.reset();
                }
            }
            // 没任务做了，执行空闲协程
            else {
                if (is_active) {
                    --m_activeThreadCount;
                    continue;
                }
                if (idle_fiber->getState() == Fiber::TERM) {
                    break;
                }

                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;

                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                    idle_fiber->setState(Fiber::HOLD);
                }    
            }
        }
    }

    void Scheduler::tickle() {
        LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping() {
        Mutex::Lock lock(m_mutex);
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle() {
        LOG_INFO(g_logger) << "idle";
        while (!stopping()) {
            Fiber::YieldToHold();
        }
    }
}