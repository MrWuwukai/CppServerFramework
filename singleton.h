#ifndef _SINGLETON_H_
#define _SINGLETON_H_

namespace Framework {

    // 模板类Singleton，用于实现单例模式
    // T是要实现单例的类类型，X是默认模板参数类型（一般不用，这里默认为void），N是另一个默认模板参数（一般不用，这里默认为0）
    template<class T, class X = void, int N = 0>
    class Singleton {
    public:
        // 静态成员函数，用于获取单例对象的指针
        static T* GetInstance() {
            // 定义一个静态的T类型对象v，保证在程序生命周期内只有一个实例
            static T v;
            // 返回该单例对象的指针
            return &v;
        }
    };

    // 模板类SingletonPtr，用于实现带智能指针的单例模式
    // T是要实现单例的类类型，X是默认模板参数类型（一般不用，这里默认为void），N是另一个默认模板参数（一般不用，这里默认为0）
    template<class T, class X = void, int N = 0>
    class SingletonPtr {
    public:
        // 静态成员函数，用于获取单例对象的智能指针
        static std::shared_ptr<T> GetInstance() {
            // 定义一个静态的std::shared_ptr<T>类型对象v，使用new创建T类型的实例并管理其生命周期
            static std::shared_ptr<T> v(new T);
            // 返回该单例对象的智能指针
            return v;
        }
    };

}

#endif