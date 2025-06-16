#include "multithread.h"
#include "log.h"
#include "utils.h"

namespace Framework {
	static thread_local Multithread* t_thread = nullptr;
	static thread_local std::string t_thread_name = "UNKNOWN";
	static Framework::Logger::ptr g_logger = LOG_NAME("system");

    Multithread* Multithread::GetThis() {
        return t_thread;
    }

    const std::string& Multithread::GetName() {
        return t_thread_name;
    }

    void Multithread::SetName(const std::string& name) {
        if (t_thread) {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    Multithread::Multithread(std::function<void()> cb, const std::string& name) : m_cb(cb){
        if (name.empty()) {
            m_name = "UNKNOWN";
        }
        else {
            m_name = name;
        }
        int rt = pthread_create(&m_thread, nullptr, &Multithread::run, this);
        if (rt) {
            LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
        m_semaphore.wait();
    }

    Multithread::~Multithread() {
        if (m_thread) {
            pthread_detach(m_thread);
        }
    }

    void Multithread::join() {
        if (m_thread) {
            int rt = pthread_join(m_thread, nullptr);
            if (rt) {
                LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt << " name=" << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }

    void* Multithread::run(void* arg) {
        Multithread* thread = (Multithread*)arg;
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = Framework::GetThreadId();
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        std::function<void()> cb;
        cb.swap(thread->m_cb);
        /*思考：为什么这里需要swap一下？*/

        thread->m_semaphore.notify();
        /*思考：这两段代码里的m_semaphore.wait();和thread->m_semaphore.notify();分别起到了什么作用？
        等待线程真正开始运行​：在调用 pthread_create 创建线程之后，主线程（即创建线程的代码所在的位置）会立即执行到 m_semaphore.wait()。此时新创建的线程可能还没有真正开始执行 run 函数。
        ​确保线程已经启动并初始化完成​：通过 wait()，主线程会阻塞在这里，直到新线程在 run 函数中调用 notify() 来释放这个信号量。这保证了主线程在继续执行之前，新线程已经完成了必要的初始化（比如设置线程 ID、线程名称等），并且已经准备好执行回调函数 cb。
        */

        cb();
        return 0;
    }
}

namespace Framework {
    Semaphore::Semaphore(uint32_t count) {
        if (sem_init(&m_semaphore, 0, count)) {
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore() {
        sem_destroy(&m_semaphore);
    }

    void Semaphore::wait() {
        if (sem_wait(&m_semaphore)) {
            throw std::logic_error("sem_wait error");
        }
    }

    void Semaphore::notify() {
        if (sem_post(&m_semaphore)) {
            throw std::logic_error("sem_post error");
        }
    }
}