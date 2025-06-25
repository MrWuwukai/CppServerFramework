#include "utils.h"
#include "fiber.h"
#include "log.h"
#include <execinfo.h>
#include <sys/time.h>

namespace Framework {
    static Framework::Logger::ptr g_logger = LOG_NAME("system");

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
        return Fiber::GetFiberId();
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


namespace Framework {
    void Backtrace(std::vector<std::string>& bt, int size, int skip) {
        // 之所以分配在堆上是因为协程的栈空间比线程小很多，所以要节省栈空间
        void* array = (void*)malloc((sizeof(void*) * size));
        size_t s = ::backtrace(array, size); // 当 :: 前面不加任何标识符​（即写成 :: 后直接跟名称）时，它表示全局作用域​（Global Scope）。
        char** strings = backtrace_symbols(array, s);

        if (strings == NULL) {
            free(strings);
            free(array);
            LOG_ERROR(g_logger) << "backtrace_symbols error";
            return;
        }

        for (size_t i = skip; i < s; ++i) {
            bt.push_back(strings[i]);
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string& prefix) {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i) {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }
}

namespace Framework {
    uint64_t GetCurrentMS() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }

    uint64_t GetCurrentUS() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }


}