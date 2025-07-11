#pragma once
#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>

#include "noncopyable.h"

namespace Framework {

    class Semaphore : private Noncopyable {
    public:
        // 构造函数，接受一个无符号 32 位整数作为信号量的初始计数值，默认为 0
        Semaphore(uint32_t count = 0);
        // 析构函数
        ~Semaphore();

        // 等待信号量，若信号量计数值大于 0，则将其减 1 并继续执行；若计数值为 0，则线程阻塞直到信号量被唤醒
        void wait();
        // 发布信号量，将信号量的计数值加 1，若有线程因等待该信号量而阻塞，则唤醒其中一个线程
        void notify();
    //private:
    //    Semaphore(const Semaphore&) = delete;
    //    Semaphore(Semaphore&&) = delete;
    //    Semaphore& operator=(const Semaphore&) = delete;
    private:
        sem_t m_semaphore; // 用于存储信号量的成员变量
    };

    template<class T>
    struct ScopedLockImpl {
    public:
        ScopedLockImpl(T& mutex)
            :m_mutex(mutex) {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };
    /*思考：这个锁的模板类为什么这么设计？RAII？*/

    template<class T>
    struct ReadScopedLockImpl {
    public:
        ReadScopedLockImpl(T& mutex)
            : m_mutex(mutex) {
            m_mutex.rdlock();
            m_locked = true;
        }

        ~ReadScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    template<class T>
    struct WriteScopedLockImpl {
    public:
        WriteScopedLockImpl(T& mutex)
            :m_mutex(mutex) {
            m_mutex.wrlock();
            m_locked = true;
        }

        ~WriteScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    class RWMutex : private Noncopyable {
    public:
        typedef ReadScopedLockImpl<RWMutex> ReadLock;
        typedef WriteScopedLockImpl<RWMutex> WriteLock;

        RWMutex() {
            pthread_rwlock_init(&m_lock, nullptr);
        }

        ~RWMutex() {
            pthread_rwlock_destroy(&m_lock);
        }

        void rdlock() {
            pthread_rwlock_rdlock(&m_lock);
        }

        void wrlock() {
            pthread_rwlock_wrlock(&m_lock);
        }

        void unlock() {
            pthread_rwlock_unlock(&m_lock);
        }
    private:
        pthread_rwlock_t m_lock;
    };

    class Mutex : private Noncopyable {
    public:
        typedef ScopedLockImpl<Mutex> Lock;
        Mutex() {
            pthread_mutex_init(&m_mutex, nullptr);
        }
        ~Mutex() {
            pthread_mutex_destroy(&m_mutex);
        }
        void lock() {
            pthread_mutex_lock(&m_mutex);
        }
        void unlock() {
            pthread_mutex_unlock(&m_mutex);
        }
    private:
        pthread_mutex_t m_mutex;
    };

    class Spinlock : private Noncopyable {
    public:
        typedef ScopedLockImpl<Spinlock> Lock;
        Spinlock() {
            pthread_spin_init(&m_mutex, 0);
        }

        ~Spinlock() {
            pthread_spin_destroy(&m_mutex);
        }

        void lock() {
            pthread_spin_lock(&m_mutex);
        }

        void unlock() {
            pthread_spin_unlock(&m_mutex);
        }
    private:
        pthread_spinlock_t m_mutex;
    };

    class CASLock : private Noncopyable {
    public:
        typedef ScopedLockImpl<CASLock> Lock;
        CASLock() {
            m_mutex.clear();
        }
        ~CASLock() {
        }
        void lock() {
            while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
        }
        void unlock() {
            std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
        }
    private:
        volatile std::atomic_flag m_mutex;
    };

    class Multithread {
    public:
        // 定义一个指向Multithread类对象的智能指针类型别名ptr
        typedef std::shared_ptr<Multithread> ptr;
        // 构造函数，接受一个无参数无返回值的函数对象cb和一个字符串name作为参数
        Multithread(std::function<void()> cb, const std::string& name);
        // 析构函数
        ~Multithread();

        // 获取线程ID的成员函数，返回线程ID
        pid_t getId() const { return m_id; }
        // 获取线程名称的成员函数，返回线程名称
        const std::string& getName() const { return m_name; }

        // 等待线程结束的成员函数
        void join();

        // 静态成员函数，获取当前线程对象指针
        static Multithread* GetThis();
        // 静态成员函数，获取当前线程名称
        static const std::string& GetName();
        static void SetName(const std::string& name);
        /*思考：这个类的GetThis、GetName为什么要设计成静态方法？*/
    private:
        // 禁用拷贝构造函数
        Multithread(const Multithread&) = delete;
        // 禁用拷贝赋值运算符
        Multithread(const Multithread&) = delete;
        Multithread& operator=(const Multithread&) = delete;

        static void* run(void* arg);
    private:
        // 线程ID
        pid_t m_id = -1;
        // POSIX线程句柄
        long int m_thread = 0; // pthread_t
        // 存储线程执行函数的函数对象
        std::function<void()> m_cb;
        // 线程名称
        std::string m_name;

        Semaphore m_semaphore;
    };

}