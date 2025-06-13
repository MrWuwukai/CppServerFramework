#pragma once
#include <thread>
#include <functional>
#include <memory>
#include <string>
#include <pthread.h>

namespace Framework {

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
    };

}