#pragma once
/*ģ��Э�̣���һ����Э�̿��ơ�����Э����Ҫ����Э�̵����л�*/
#include <ucontext.h> // win��ʹ��#include <windows.h>��fiber
#include <memory>
#include <functional>
#include "multithread.h"
#include "log.h"

namespace Framework {
    class Fiber : public std::enable_shared_from_this<Fiber> {
    public:
        typedef std::shared_ptr<Fiber> ptr;

        enum State {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };

        Fiber(std::function<void()> cb, size_t stacksize = 0);
        ~Fiber();

        //����Э�̺�����������״̬
        //INIT, TERM
        void reset(std::function<void()> cb);
        //�л�����ǰЭ��ִ��
        void swapIn();
        //�л�����ִ̨��
        void swapOut();
        //���õ�ǰЭ��
        static void SetThis(Fiber* f);
        //���ص�ǰЭ��
        static Fiber::ptr GetThis();

		uint64_t getId() const {
			return m_id;
		}

        static uint64_t GetFiberId();
        //Э���л�����̨����������ΪReady״̬
        static void YieldToReady();
        //Э���л�����̨����������ΪHold״̬
        static void YieldToHold();
        //��Э����
        static uint64_t TotalFibers();

        static void MainFunc();
    private:
        Fiber(); // ˼����Ϊʲô�޲ι���Ҫд��˽�У�
    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;

        ucontext_t m_ctx;
        void* m_stack = nullptr;

        std::function<void()> m_cb;
    };
}