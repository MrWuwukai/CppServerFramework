#pragma once
#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <pthread.h>

namespace Framework {

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
    };

}