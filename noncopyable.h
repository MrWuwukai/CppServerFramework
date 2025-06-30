#pragma once
namespace Framework {
    class Noncopyable {
    public:
        Noncopyable() = default;
        virtual ~Noncopyable() = default;
        Noncopyable(const Noncopyable&) = delete;
        // ��ֵ�����������ʱ����
        // Noncopyable(const Noncopyable&&) = delete;
        Noncopyable& operator=(const Noncopyable&) = delete;
    };
}