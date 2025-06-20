#include <vector>
#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace Framework {
	static Logger::ptr g_logger = LOG_NAME("system");

	static thread_local Scheduler* t_scheduler = nullptr; // ��ǰ�̹߳����ĵ�����
	static thread_local Fiber* t_fiber = nullptr; // ��ǰ�߳�����ִ�е�Э��(��ǰЭ��)

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
        :m_name(name){
        ASSERT(threads > 0);

        if (use_caller) {
            Fiber::GetThis(); // ���߳�Ҳ���ܵ��ȣ����Ի�ȡ��û���򴴽���������̵߳���Э��
            --threads; // ���߳�Ҳ���ܵ��ȣ���������һ���߳�

            ASSERT(GetThis() == nullptr); // ȷ�����̲߳����Ѿ�ӵ��һ��������
            t_scheduler = this; // ��ǰ�̵߳ĵ������������ڹ������� Scheduler ����
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true)); // ���õ�������Э��
            Multithread::SetName(m_name);
            
            t_fiber = m_rootFiber.get(); // ��ǰ�߳�����ִ�е�Э�̾��ǵ�������Э��
            m_rootThread = GetThreadId(); // ���õ��������߳�
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
        // �����̳߳أ���Ҫ��ǰδ�������̳߳�Ϊ��
        Mutex::Lock lock(m_mutex);
        if (!m_stopping) {
            return;
        }
        m_stopping = false;
        ASSERT(m_threads.empty());

        // ע���̵߳��̳߳أ�ͳһִ��run����
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Multithread(std::bind(&Scheduler::run, this)
                , m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId()); // �̲߳��ź������Ʊ��ܻ��id�����߳�ͷ�ļ���
        }
        //lock.unlock();

        /*if (m_rootFiber){
            m_rootFiber->call();
	    }*/
    }

    void Scheduler::stop() {
        // ����������ִ����Ϻ�ֹͣ�����̳߳�
        m_autoStop = true;

        // ���̳߳���û���̣߳�ֻ��һ����������Э��
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

        // ���̳߳������̣߳������̲߳��������Զ����������use_caller������߳���Ҫ���⴦��
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
        // ���õ�ǰ������Ϊ���߳�����������
        setThis();
        // �����ǰ�̲߳������̣߳�����һ�µ�ǰ�̵߳�����ִ��run��Э��
        if (Framework::GetThreadId() != m_rootThread) {
            t_fiber = Fiber::GetThis().get();
        }

        // ��������Э�̣��������������񡢻���Ƚ���֮�󣬾ͻ�ִ�п���Э�̣�����ʱ�ø�ʲôȡ���ھ���ʵ��
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while (true) {
            // ����FiberAndThread����
            ft.reset();
            // ����Ƿ����߳���Ҫ������
            bool tickle_me = false;
            bool is_active = false;
            // ��fibers��������ȡ��һ�������ft
            {
                // ������������������m_fibers�����ķ���
                Mutex::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end()) {
                    // ���ָ��ִ�е��̲߳��Ǳ��̣߳�����Ҫ�����Ǹ��߳�
                    if (it->thread != -1 && it->thread != Framework::GetThreadId()) {
                        // �ƶ�����һ������
                        ++it;
                        // �����Ҫ���������߳�
                        tickle_me = true;
                        continue;
                    }
                    // ���ԣ�����Ҫô���˳̣�Ҫô�лص�����
                    ASSERT(it->fiber || it->cb);
                    // ����������˳����˳̴���ִ��״̬���Ѿ�ִ�оͲ���Ҫִ����
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                        // �ƶ�����һ������
                        ++it;
                        continue;
                    }
                    // ����ǰ����ֵ��ft
                    ft = *it;
                    // �������б����Ƴ�������
                    m_fibers.erase(it);
                    ++m_activeThreadCount;
                    is_active = true;

                    break;
                }
            }
            // ������߳���Ҫ������
            if (tickle_me) {
                // ִ�л��Ѳ���
                tickle();
            }
            // ���ft�����˳����˳�״̬������ֹ״̬
            if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {              
                // ���˳��л�����ִ��
                ft.fiber->swapIn();
                --m_activeThreadCount;

                // ����˳�״̬��Ϊ����
                if (ft.fiber->getState() == Fiber::READY) {
                    // �����ӵ�������
                    schedule(ft.fiber);
                }
                // ����˳�״̬�Ȳ�����ֹ״̬Ҳ�����쳣״̬
                else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                    // ���˳�״̬����Ϊ����
                    ft.fiber->setState(Fiber::HOLD);
                }
                // ����ft
                ft.reset();
            }
            // �������Fiber������ͨ�ص�
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
            // û�������ˣ�ִ�п���Э��
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