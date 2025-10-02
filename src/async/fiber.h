#pragma once

/*模拟协程，由一个主协程控制。各个协程需要由主协程调度切换*/
#include <ucontext.h> // win下使用#include <windows.h>的fiber

#include <functional>
#include <memory>

#include "log.h"
#include "multithread.h"

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

        Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
        ~Fiber();

        //重置协程函数，并重置状态
        //INIT, TERM
        void reset(std::function<void()> cb);
        //切换到当前协程执行
        void swapIn();
        //切换到后台执行
        void swapOut();
        // 直接切换协程，不需要通过主协程
        void call();
        void uncall();

        //设置当前协程
        static void SetThis(Fiber* f);
        //返回当前协程
        static Fiber::ptr GetThis();

		uint64_t getId() const {
			return m_id;
		}
        Fiber::State getState() const {
            return m_state;
        }
        void setState(const State state) {
            m_state = state;
        }

        static uint64_t GetFiberId();
        //协程切换到后台，并且设置为Ready状态
        static void YieldToReady();
        //协程切换到后台，并且设置为Hold状态
        static void YieldToHold();
        //总协程数
        static uint64_t TotalFibers();

        static void MainFunc();
        static void MainFuncCaller();
    private:
        Fiber(); // 思考：为什么无参构造要写成私有？
    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;

        ucontext_t m_ctx;
        void* m_stack = nullptr;

        std::function<void()> m_cb;
    };
}