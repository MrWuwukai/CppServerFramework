#include "utils.h"

namespace Framework {
    uint32_t GetThreadId() {
    #ifdef _WIN32
        std::thread::id tid = std::this_thread::get_id();
        std::hash<std::thread::id> hasher;
        size_t id_hash = hasher(tid); // 得到一个数值哈希值

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
        // 使用 std::hash 计算指针的哈希值
        std::hash<LPVOID> hasher;
        size_t hash_value = hasher(fiber);
        // 将哈希值转换为 uint32_t
        uint32_t fiber_id = static_cast<uint32_t>(hash_value);
        return fiber_id;
    #else
        // Linux 平台：使用 Boost.Fiber 获取当前 Fiber ID
        // 注意：必须在 Boost.Fiber 环境下运行，否则会崩溃
        try {
            boost::fibers::fiber::id fiber_id = boost::this_fiber::get_id();
            // 将 fiber_id 转换为 uint32_t（Boost 的 fiber::id 可以隐式转换为 size_t）
            uint32_t id_num = static_cast<uint32_t>(fiber_id);
            return id_num;
        }
        catch (...) {
            // 如果不在 Boost.Fiber 环境下调用此函数，可能会抛出异常
            // 返回 0 表示无效 Fiber ID
            return 0;
        }
    #endif
    }

    #ifdef _WIN32
    int myvasprintf(char** strp, const char* format, va_list ap) {
        
        int wanted = vsnprintf(*strp = NULL, 0, format, ap);
        if ((wanted > 0) && ((*strp = (char*)malloc(1 + wanted)) != NULL))
            return vsprintf(*strp, format, ap);

        return wanted;
    }
    #endif
}