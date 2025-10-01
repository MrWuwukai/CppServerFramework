#pragma once
namespace Framework {
    class Noncopyable {
    public:
        Noncopyable() = default;
        virtual ~Noncopyable() = default;
        Noncopyable(const Noncopyable&) = delete;
        // 右值的这个拷贝暂时不禁
        // Noncopyable(const Noncopyable&&) = delete;
        Noncopyable& operator=(const Noncopyable&) = delete;
    };
}