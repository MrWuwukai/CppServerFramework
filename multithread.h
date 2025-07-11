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
        // ���캯��������һ���޷��� 32 λ������Ϊ�ź����ĳ�ʼ����ֵ��Ĭ��Ϊ 0
        Semaphore(uint32_t count = 0);
        // ��������
        ~Semaphore();

        // �ȴ��ź��������ź�������ֵ���� 0������� 1 ������ִ�У�������ֵΪ 0�����߳�����ֱ���ź���������
        void wait();
        // �����ź��������ź����ļ���ֵ�� 1�������߳���ȴ����ź�������������������һ���߳�
        void notify();
    //private:
    //    Semaphore(const Semaphore&) = delete;
    //    Semaphore(Semaphore&&) = delete;
    //    Semaphore& operator=(const Semaphore&) = delete;
    private:
        sem_t m_semaphore; // ���ڴ洢�ź����ĳ�Ա����
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
    /*˼�����������ģ����Ϊʲô��ô��ƣ�RAII��*/

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
        // ����һ��ָ��Multithread����������ָ�����ͱ���ptr
        typedef std::shared_ptr<Multithread> ptr;
        // ���캯��������һ���޲����޷���ֵ�ĺ�������cb��һ���ַ���name��Ϊ����
        Multithread(std::function<void()> cb, const std::string& name);
        // ��������
        ~Multithread();

        // ��ȡ�߳�ID�ĳ�Ա�����������߳�ID
        pid_t getId() const { return m_id; }
        // ��ȡ�߳����Ƶĳ�Ա�����������߳�����
        const std::string& getName() const { return m_name; }

        // �ȴ��߳̽����ĳ�Ա����
        void join();

        // ��̬��Ա��������ȡ��ǰ�̶߳���ָ��
        static Multithread* GetThis();
        // ��̬��Ա��������ȡ��ǰ�߳�����
        static const std::string& GetName();
        static void SetName(const std::string& name);
        /*˼����������GetThis��GetNameΪʲôҪ��Ƴɾ�̬������*/
    private:
        // ���ÿ������캯��
        Multithread(const Multithread&) = delete;
        // ���ÿ�����ֵ�����
        Multithread(const Multithread&) = delete;
        Multithread& operator=(const Multithread&) = delete;

        static void* run(void* arg);
    private:
        // �߳�ID
        pid_t m_id = -1;
        // POSIX�߳̾��
        long int m_thread = 0; // pthread_t
        // �洢�߳�ִ�к����ĺ�������
        std::function<void()> m_cb;
        // �߳�����
        std::string m_name;

        Semaphore m_semaphore;
    };

}