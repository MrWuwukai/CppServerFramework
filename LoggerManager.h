#pragma once
#include "log.h"
#include "singleton.h"

namespace Framework {
    class LoggerManager {
    public:
        LoggerManager();
        Logger::ptr getLogger(const std::string& name);

        void init();
    private:
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    typedef Framework::Singleton<LoggerManager> loggerMgr;
}