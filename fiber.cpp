#include "fiber.h"
#include "config.h"
#include "macro.h"
#include <atomic>

namespace Framework {
    static Framework::Logger::ptr g_logger = LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{ 0 };
    static std::atomic<uint64_t> s_fiber_count{ 0 };
    static thread_local Fiber* t_fiber = nullptr; // �߳̿���
    static thread_local Fiber::ptr t_threadFiber = nullptr; // mainЭ��
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size"); // ����һ��Э��ջ�ռ��С

	// ͨ��ջ�ڴ������
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

    // Э�̳�ʼ��������߳���������Ϣ
    // ��Э��û��stack
    Fiber::Fiber() {
        m_state = EXEC;
        SetThis(this);

        if (getcontext(&m_ctx)) {
            ASSERT_W(false, "getcontext");
        }

        ++s_fiber_count;
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize)
        : m_id(++s_fiber_id)  // ��ʼ��Э�̵�ΨһID��ʹ�þ�̬��Ա����s_fiber_id����������Ψһ��ʶ
        , m_cb(cb) {  // ���洫���������
        ++s_fiber_count;  // Э�̼�������������¼��ǰ������Э������

        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();  // ����ջ��С������д�����ʹ�ô���ֵ������ʹ��ȫ��Ĭ��ջ��С

        m_stack = myStackAllocator::Alloc(m_stacksize);  // ��ջ�������з���ָ����С��ջ�ڴ�
        if (getcontext(&m_ctx)) {  // ��ȡ��ǰ������
            ASSERT_W(false, "getcontext");  // �����ȡ������ʧ�ܣ�����ʧ�ܲ����������Ϣ
        }
        m_ctx.uc_link = nullptr;  // ���������ĵ�����Ϊnullptr����ʾ��ǰ������ִ����Ϻ�û�к���������
        m_ctx.uc_stack.ss_sp = m_stack;  // ���������ĵ�ջָ��Ϊ�����ջ�ڴ���ʼ��ַ
        m_ctx.uc_stack.ss_size = m_stacksize;  // ���������ĵ�ջ��С

        makecontext(&m_ctx, &Fiber::MainFunc, 0);  // ��ʼ�������ģ�����ִ�к���ΪFiber::MainFunc����������Ϊ0
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

    // Э�̹ر�ʱ�ڴ���ʱ���ͷţ�����Դֱ��ת�����µ�Э��
    void Fiber::reset(std::function<void()> cb) {
        ASSERT(m_stack); // ���Բ�������Э��
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

    // �л�ִ�б�Э�̣�����ǰִ�е�Э�̷ŵ���̨
    void Fiber::swapIn() {
        SetThis(this);
        ASSERT(m_state != EXEC);
        m_state = EXEC;
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
            ASSERT_W(false, "swapcontext");
        }
    }

    void Fiber::swapOut() {
        SetThis(t_threadFiber.get());

        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            ASSERT_W(false, "swapcontext");
        }
    }

    //���õ�ǰЭ��
    void Fiber::SetThis(Fiber* f) {
        t_fiber = f;
    }

    //���ص�ǰЭ��
    Fiber::ptr Fiber::GetThis() {
        if (t_fiber)
            return t_fiber->shared_from_this();

        // ��Э��
        Fiber::ptr main_fiber(new Fiber);
        ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    uint64_t Fiber::GetFiberId() {
        if (t_fiber) { // û���κ�Э�̵��̷߳���0
            return t_fiber->getId();
        }
        return 0;
    }

    // ���õ�ǰЭ��Ϊready������л�����Э��
    void Fiber::YieldToReady() {
        Fiber::ptr cur = GetThis();
        cur->m_state = READY;
        cur->swapOut();
    }

    // Э���л�����̨����������ΪHold״̬
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
            cur->m_cb = nullptr; // ָ���ÿգ���ֹ���󶨲�����������ָ�����ô���һֱ+1; ��ʹ m_cb û��ֱ�Ӳ��� Fiber������ص������ڲ�ͨ��������ʽ����ȫ�ֱ�������̬�����ȣ���������� Fiber ����Ҳ������ֹ��������
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

        // cur->swapOut(); // �ֶ�������Э�̣����Ǵ�ʱFiber::ptr cur = GetThis();���������ָ�벢δ���ͷ����ü����ڴ˴�һ��>=1
        auto raw_ptr = cur.get(); // ��Ҫʹ����ָ��
        cur.reset();
        cur->swapOut();
    }
}