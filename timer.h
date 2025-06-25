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
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager); // ˽�й��죬������ֱ��ʹ��Timer��������Ҫmanager����
        Timer(uint64_t next);       
    private:
        bool m_recurring = false;        //�Ƿ�ѭ����ʱ��
        uint64_t m_ms = 0;               //ִ������
        uint64_t m_next = 0;             //��һ��Ӧ�ñ������ľ���ʱ��
        std::function<void()> m_cb;
        TimerManager* m_manager = nullptr;
    private:
        struct Comparator {
            bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const; // �趨һ���º���ר�����ڱȽ�
        };
    };

    class TimerManager {
        friend class Timer;
    public:
        TimerManager();
        virtual ~TimerManager();

        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
        void addTimer(Timer::ptr p, RWMutex::WriteLock& lock);
        // ������ʱ�������������Ŵ�����weak_ptr���ü���Ϊ0ʱ����������ʧЧ
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);
        uint64_t getNextTimer();
        void listExpiredCb(std::vector<std::function<void()> >& cbs); // �г��Ѿ���ʱ�����ִ�еĶ�ʱ��
    protected:
        virtual void onTimerInsertedAtFront() = 0;
        bool hasTimer();
    private:
        bool detectClockRollover(uint64_t now_ms); // ��ֹ������ͻȻ�޸�ʱ��
    private:
        RWMutex m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers; // ������Timer������set��Ĭ��������ָ���ַ������Ҫ�趨�������
        bool m_tickled = false;
        uint64_t m_previous = 0;
    };
}