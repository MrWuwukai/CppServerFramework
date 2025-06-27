#include "hook.h"
#include <dlfcn.h> // dlsym

namespace Framework {

    static thread_local bool t_hook_enable = false; // 线程级的hook

    #define HOOK_FUN(XX) \
        XX(sleep) \
        XX(usleep)

    void hook_init() {
        static bool is_inited = false;
        if (is_inited) {
            return;
        }
        // 使用 dlsym 动态链接库函数，查找函数地址
        // 例如找到sleep，强转成sleep_fun之后赋值给sleep_f
        // 拦截某个函数的调用（如 sleep 或 usleep），并替换成自定义的实现
        // dlsym(RTLD_NEXT, "sleep") 会查找下一个​（即系统库中的）sleep 函数地址，而不是当前模块中的 sleep 函数。
        // 这样，sleep_f 就指向了系统库的 sleep 函数，而不是当前模块的 sleep 函数。
        // 然后，你可以在自己的代码中提供一个自定义的 sleep 函数，并在调用 sleep_f 之前 / 之后插入额外的逻辑（如日志记录、性能统计等）。
        #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
        // 此处被替换后，sleep变成了我的sleep，sleep_f才是原始sleep
        #undef XX
    }

    // 静态全局变量在main前初始化（会调用构造函数），确保先hook
    // __attribute__((constructor))也能实现类似效果
    struct _HookIniter {
        _HookIniter() {
            hook_init();
        }
    };
    static _HookIniter s_hook_initer;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }
}

extern "C" {
    #define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
    #undef XX

    unsigned int sleep(unsigned int seconds) {
        if (!Framework::t_hook_enable) {
            return sleep_f(seconds);
        }
        Framework::Fiber::ptr fiber = Framework::Fiber::GetThis();
        Framework::IOManager* iom = Framework::IOManager::GetThis();
        iom->addTimer(seconds * 1000, [iom, fiber]() {iom->schedule(fiber); });
        Framework::Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec) {
        if (!Framework::t_hook_enable) {
            return usleep_f(usec);
        }
        Framework::Fiber::ptr fiber = Framework::Fiber::GetThis();
        Framework::IOManager* iom = Framework::IOManager::GetThis();
        iom->addTimer(usec / 1000, [iom, fiber]() {iom->schedule(fiber); });
        Framework::Fiber::YieldToHold();
        return 0;
    }
}