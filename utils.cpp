#include "utils.h"

namespace Framework {
    uint32_t GetThreadId() {
    #ifdef _WIN32
        std::thread::id tid = std::this_thread::get_id();
        std::hash<std::thread::id> hasher;
        size_t id_hash = hasher(tid); // �õ�һ����ֵ��ϣֵ

        uint32_t id_num = static_cast<uint32_t>(id_hash);
        return id_num;
    }
    #else
    uint32_t GetThreadId() {
        return (uint32_t)syscall(SYS_gettid);
    }
    #endif

    uint32_t GetFiberId() {
    #ifdef _WIN32
        LPVOID fiber = GetCurrentFiber();
        if (fiber == nullptr) {
            return 0;
        }
        // ʹ�� std::hash ����ָ��Ĺ�ϣֵ
        std::hash<LPVOID> hasher;
        size_t hash_value = hasher(fiber);
        // ����ϣֵת��Ϊ uint32_t
        uint32_t fiber_id = static_cast<uint32_t>(hash_value);
        return fiber_id;
    #else
        // Linux ƽ̨��ʹ�� Boost.Fiber ��ȡ��ǰ Fiber ID
        // ע�⣺������ Boost.Fiber ���������У���������
        try {
            boost::fibers::fiber::id fiber_id = boost::this_fiber::get_id();
            // �� fiber_id ת��Ϊ uint32_t��Boost �� fiber::id ������ʽת��Ϊ size_t��
            uint32_t id_num = static_cast<uint32_t>(fiber_id);
            return id_num;
        }
        catch (...) {
            // ������� Boost.Fiber �����µ��ô˺��������ܻ��׳��쳣
            // ���� 0 ��ʾ��Ч Fiber ID
            return 0;
        }
    #endif
    }
}