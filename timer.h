#pragma once
#include <memory>
#include <set>
#include "multithread.h"

namespace Framework {
    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;
    public:
        typedef std::shared_ptr<Timer> ptr;
        bool cancel();
        bool refresh();
        bool reset(uint64_t ms, bool from_now);
    private:
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager); // 私有构造，不允许直接使用Timer，而是需要manager构造
        Timer(uint64_t next);       
    private:
        bool m_recurring = false;        //是否循环定时器
        uint64_t m_ms = 0;               //执行周期
        uint64_t m_next = 0;             //下一次应该被触发的绝对时间
        std::function<void()> m_cb;
        TimerManager* m_manager = nullptr;
    private:
        struct Comparator {
            bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const; // 设定一个仿函数专门用于比较
        };
    };

    class TimerManager {
        friend class Timer;
    public:
        TimerManager();
        virtual ~TimerManager();

        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
        void addTimer(Timer::ptr p, RWMutex::WriteLock& lock);
        // 条件定时器，满足条件才触发，weak_ptr引用计数为0时该条件变量失效
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);
        uint64_t getNextTimer();
        void listExpiredCb(std::vector<std::function<void()> >& cbs); // 列出已经到时间可以执行的定时器
    protected:
        virtual void onTimerInsertedAtFront() = 0;
        bool hasTimer();
    private:
        bool detectClockRollover(uint64_t now_ms); // 防止服务器突然修改时间
    private:
        RWMutex m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers; // 有序存放Timer，但是set的默认排序按照指针地址排序，需要设定排序规则
        bool m_tickled = false;
        uint64_t m_previous = 0;
    };
}