#include "timer.h"
#include "utils.h"

namespace Framework {
    bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
        // 比较两个定时器，谁执行时间短就小，执行时间一样就比较地址大小
        if (!lhs && !rhs) {
            return false;
        }
        if (!lhs) {
            return true;
        }
        if (!rhs) {
            return false;
        }
        if (lhs->m_next < rhs->m_next) {
            return true;
        }
        if (rhs->m_next < lhs->m_next) {
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
        m_next = Framework::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next)
        : m_next(next) {
    }

    TimerManager::TimerManager() {
        m_previous = Framework::GetCurrentMS();
    }

    TimerManager::~TimerManager() {
    }

    bool Timer::cancel() {
        RWMutex::WriteLock lock(m_manager->m_mutex);
        if (m_cb) {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh() {
        RWMutex::WriteLock lock(m_manager->m_mutex);
        if (!m_cb) {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it); // 先删除再重新设定时间，不然set内部排序会乱
        m_next = Framework::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now) {
        RWMutex::WriteLock lock(m_manager->m_mutex);
        if (ms == m_ms && !from_now) {
            return true;
        }        
        if (!m_cb) {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it); // 先删除再重新设定时间，不然set内部排序会乱

        uint64_t start = 0;
        // 如果from_now，则下一次的执行时间为now+新的间隔，不然的话原定下一次执行的时间不变
        if (from_now) {
            start = Framework::GetCurrentMS();
        }
        else {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }
}


namespace Framework {
    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool _recurring) {
        Timer::ptr timer(new Timer(ms, cb, _recurring, this));
        RWMutex::WriteLock lock(m_mutex);
        addTimer(timer, lock);

        return timer;
    }

    void TimerManager::addTimer(Timer::ptr timer, RWMutex::WriteLock& lock) {
        auto it = m_timers.insert(timer).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled; // 插入的迭代器位置在最前，说明插入的定时器最小
        
        // if (at_front) {
        //     m_tickled = true;
        //     lock.unlock();
        //     onTimerInsertedAtFront(); // 插入的定时器最小，说明之前的定时器太老了，需要唤醒调度器立刻重设时间
        // }
        // lock.unlock();
        if(at_front) {
            m_tickled = true;
        }
        lock.unlock();
        if(at_front) {
            onTimerInsertedAtFront();
        }
    }

    // 定义一个静态函数OnTimer，它接收一个弱指针weak_cond和一个函数对象cb作为参数
    // 弱指针用于在不增加引用计数的情况下观察对象，函数对象cb是要执行的回调操作
    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
        // 尝试将弱指针weak_cond转换为强指针tmp，如果转换成功说明被观察的对象仍然存在
        std::shared_ptr<void> tmp = weak_cond.lock();
        // 如果tmp不为空，即弱指针对应的对象还存在
        if (tmp) {
            // 执行回调函数cb
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring) {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer() {
        RWMutex::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty()) {
            return ~0ull;
        }
        const Timer::ptr& next = *m_timers.begin();
        uint64_t now_ms = Framework::GetCurrentMS();
        if (now_ms >= next->m_next) { // 因为某些原因当前时间已经超过了下一个定时器的时间，则需要立刻执行下一个定时器
            return 0;
        }
        else {
            return next->m_next - now_ms; // 不然的话，就可以等待一段时间
        }
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
        uint64_t now_ms = Framework::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        
        RWMutex::WriteLock lock(m_mutex);
		if (m_timers.empty()) {
			return;
		}
         
        bool rollover = detectClockRollover(now_ms);
        if (!rollover && (*m_timers.begin())->m_next > now_ms) {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer); // >now_timer的定时器
        expired.assign(m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);

        for (auto& timer : expired) {
            cbs.push_back(timer->m_cb); // 回调
            if (timer->m_recurring) { // 循环定时器重设时间后再给他塞回去
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else {
                timer->m_cb = nullptr; // 置空m_cb，确保智能指针引用计数-1
            }
        }
    }

    bool TimerManager::hasTimer() {
        RWMutex::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

	bool TimerManager::detectClockRollover(uint64_t now_ms) {
		bool rollover = false;
		if (now_ms < (m_previous - (uint64_t)(60 * 60 * 1000))) {
			rollover = true;
		}
		m_previous = now_ms;
		return rollover;
	}
}