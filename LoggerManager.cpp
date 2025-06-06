#include "LoggerManager.h"

namespace Framework {
    LoggerManager::LoggerManager() {
        m_root.reset(new Logger);
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    }

    Logger::ptr LoggerManager::getLogger(const std::string& name) {
        auto it = m_loggers.find(name);
        return it == m_loggers.end() ? m_root : it->second;
    }
}