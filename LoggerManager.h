#pragma once
#include "log.h"
#include "singleton.h"

#define LOG_ROOT() Framework::loggerMgr::GetInstance()->getRoot()

namespace Framework {
    class LoggerManager {
    public:
        LoggerManager();
        Logger::ptr getLogger(const std::string& name);
        Logger::ptr getRoot() const { return m_root; }

        void init();
    private:
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    typedef Framework::Singleton<LoggerManager> loggerMgr;
}