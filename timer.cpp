#include "timer.h"
#include "utils.h"

namespace Framework {
    bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
        // �Ƚ�������ʱ����˭ִ��ʱ��̾�С��ִ��ʱ��һ���ͱȽϵ�ַ��С
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
        m_manager->m_timers.erase(it); // ��ɾ���������趨ʱ�䣬��Ȼset�ڲ��������
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
        m_manager->m_timers.erase(it); // ��ɾ���������趨ʱ�䣬��Ȼset�ڲ��������

        uint64_t start = 0;
        // ���from_now������һ�ε�ִ��ʱ��Ϊnow+�µļ������Ȼ�Ļ�ԭ����һ��ִ�е�ʱ�䲻��
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
        bool at_front = (it == m_timers.begin() && !m_tickled); // ����ĵ�����λ������ǰ��˵������Ķ�ʱ����С
        
        if (at_front) {
            m_tickled = true;
            lock.unlock();
            onTimerInsertedAtFront(); // ����Ķ�ʱ����С��˵��֮ǰ�Ķ�ʱ��̫���ˣ���Ҫ���ѵ�������������ʱ��
        }
        lock.unlock();
    }

    // ����һ����̬����OnTimer��������һ����ָ��weak_cond��һ����������cb��Ϊ����
    // ��ָ�������ڲ��������ü���������¹۲���󣬺�������cb��Ҫִ�еĻص�����
    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
        // ���Խ���ָ��weak_condת��Ϊǿָ��tmp�����ת���ɹ�˵�����۲�Ķ�����Ȼ����
        std::shared_ptr<void> tmp = weak_cond.lock();
        // ���tmp��Ϊ�գ�����ָ���Ӧ�Ķ��󻹴���
        if (tmp) {
            // ִ�лص�����cb
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
        if (now_ms >= next->m_next) { // ��ΪĳЩԭ��ǰʱ���Ѿ���������һ����ʱ����ʱ�䣬����Ҫ����ִ����һ����ʱ��
            return 0;
        }
        else {
            return next->m_next - now_ms; // ��Ȼ�Ļ����Ϳ��Եȴ�һ��ʱ��
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
        auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer); // >now_timer�Ķ�ʱ��
        expired.assign(m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);

        for (auto& timer : expired) {
            cbs.push_back(timer->m_cb); // �ص�
            if (timer->m_recurring) { // ѭ����ʱ������ʱ����ٸ�������ȥ
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else {
                timer->m_cb = nullptr; // �ÿ�m_cb��ȷ������ָ�����ü���-1
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