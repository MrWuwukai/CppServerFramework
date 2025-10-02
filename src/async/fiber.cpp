#include <atomic>

#include "config.h"
#include "fiber.h"
#include "macro.h"
#include "scheduler.h"

namespace Framework {
    static Framework::Logger::ptr g_logger = LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{ 0 };
    static std::atomic<uint64_t> s_fiber_count{ 0 };
    static thread_local Fiber* t_fiber = nullptr; // 线程控制
    static thread_local Fiber::ptr t_threadFiber = nullptr; // main协程
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size"); // 配置一个协程栈空间大小

	// 通用栈内存分配器
    class MallocStackAllocator {
    public:
        static void* Alloc(size_t size) {
            return malloc(size);
        }

        static void Dealloc(void* vp, size_t size) {
            return free(vp);
        }
    };
    typedef MallocStackAllocator myStackAllocator;

    // 协程初始化，获得线程上下文信息
    // 主协程没有stack
    Fiber::Fiber() {
        m_state = EXEC;
        SetThis(this);

        if (getcontext(&m_ctx)) {
            ASSERT_W(false, "getcontext");
        }

        ++s_fiber_count;
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
        : m_id(++s_fiber_id)  // 初始化协程的唯一ID，使用静态成员变量s_fiber_id自增来生成唯一标识
        , m_cb(cb) {  // 保存传入的任务函数
        ++s_fiber_count;  // 协程计数器自增，记录当前创建的协程数量

        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();  // 设置栈大小，如果有传入则使用传入值，否则使用全局默认栈大小

        m_stack = myStackAllocator::Alloc(m_stacksize);  // 从栈分配器中分配指定大小的栈内存
        if (getcontext(&m_ctx)) {  // 获取当前上下文
            ASSERT_W(false, "getcontext");  // 如果获取上下文失败，断言失败并输出错误信息
        }
        m_ctx.uc_link = nullptr;  // 设置上下文的链接为nullptr，表示当前上下文执行完毕后没有后续上下文
        m_ctx.uc_stack.ss_sp = m_stack;  // 设置上下文的栈指针为分配的栈内存起始地址
        m_ctx.uc_stack.ss_size = m_stacksize;  // 设置上下文的栈大小

        if (use_caller){
            makecontext(&m_ctx, &Fiber::MainFunc, 0);  // 初始化上下文，设置执行函数为Fiber::MainFunc，参数个数为0
        }
        else {
            makecontext(&m_ctx, &Fiber::MainFuncCaller, 0);
        }
    }

    Fiber::~Fiber() {
        --s_fiber_count;
        if (m_stack) {
            ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

            myStackAllocator::Dealloc(m_stack, m_stacksize);
        }
        else {
            ASSERT(!m_cb);
            ASSERT(m_state == EXEC);

            Fiber* cur = t_fiber;
            if (cur == this) {
                SetThis(nullptr);
            }
        }
    }

    // 协程关闭时内存暂时不释放，将资源直接转交给新的协程
    void Fiber::reset(std::function<void()> cb) {
        ASSERT(m_stack); // 断言不能是主协程
        ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
        m_cb = cb;
        if (getcontext(&m_ctx)) {
            ASSERT_W(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    // 切换执行本协程，将当前执行的协程放到后台
    void Fiber::swapIn() {
        SetThis(this);
        ASSERT(m_state != EXEC);
        m_state = EXEC;
        if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
            ASSERT_W(false, "swapcontext");
        }
    }

	void Fiber::swapOut() {
		SetThis(Scheduler::GetMainFiber());
		if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
			ASSERT_W(false, "swapcontext");
		}
	}

    void Fiber::call() {
        SetThis(this);
        m_state = EXEC;
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
			ASSERT_W(false, "swapcontext");
		}
    }

	void Fiber::uncall() {
		SetThis(t_threadFiber.get());
		if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
			ASSERT_W(false, "swapcontext");
		}

	}

    //设置当前协程
    void Fiber::SetThis(Fiber* f) {
        t_fiber = f;
    }

    //返回当前协程
    Fiber::ptr Fiber::GetThis() {
        if (t_fiber)
            return t_fiber->shared_from_this();

        // 主协程
        Fiber::ptr main_fiber(new Fiber);
        ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    uint64_t Fiber::GetFiberId() {
        if (t_fiber) { // 没有任何协程的线程返回0
            return t_fiber->getId();
        }
        return 0;
    }

    // 设置当前协程为ready，随后切换到主协程
    void Fiber::YieldToReady() {
        Fiber::ptr cur = GetThis();
        cur->m_state = READY;
        cur->swapOut();
    }

    // 协程切换到后台，并且设置为Hold状态
    void Fiber::YieldToHold() {
        Fiber::ptr cur = GetThis();
        cur->m_state = HOLD;
        cur->swapOut();
    }
   
    uint64_t Fiber::TotalFibers() {
        return s_fiber_count;
    }

    void Fiber::MainFunc() {
        Fiber::ptr cur = GetThis();
        ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr; // 指针置空，防止被绑定参数导致智能指针引用次数一直+1; 即使 m_cb 没有直接捕获 Fiber，如果回调函数内部通过其他方式（如全局变量、静态变量等）间接引用了 Fiber 对象，也可能阻止其析构。
            cur->m_state = TERM;
        }
        catch (std::exception& ex) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
        }
        catch (...) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber Except";
        }

        // cur->swapOut(); // 手动返回主协程，但是此时Fiber::ptr cur = GetThis();申请的智能指针并未被释放引用计数在此处一定>=1
        auto raw_ptr = cur.get(); // 需要使用裸指针
        cur.reset();
        raw_ptr->swapOut();
    }

    void Fiber::MainFuncCaller() {
        Fiber::ptr cur = GetThis();
        ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception& ex) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
        }
        catch (...) {
            cur->m_state = EXCEPT;
            LOG_ERROR(g_logger) << "Fiber Except";
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->uncall();
    }
}