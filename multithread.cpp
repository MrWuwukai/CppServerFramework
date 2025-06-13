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

        cb();
        return 0;
    }
}