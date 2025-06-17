#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <tuple>
#include <map>
#include <functional>
#include <cstdarg>
#include <format>
#include "utils.h"
#include "singleton.h"
#include "multithread.h"

#ifdef _WIN32
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctime>
#include <cstdio> 
#else
#include <time.h>
#include <string.h>
#endif

#ifdef ERROR
#undef ERROR  // 取消宏定义
#endif

#define LOG_LEVEL(logger, level) Framework::LogEventWrap(Framework::LogEvent::ptr(new Framework::LogEvent(logger, level, __FILE__, __LINE__, 0, Framework::GetThreadId(), Framework::Multithread::GetName(), Framework::GetFiberId(), time(0)))).getSS()
#define LOG_DEBUG(logger) LOG_LEVEL(logger, Framework::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, Framework::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, Framework::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, Framework::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, Framework::LogLevel::FATAL)

#define LOG_FMT_LEVEL(logger, level, fmt, ...) Framework::LogEventWrap(Framework::LogEvent::ptr(new Framework::LogEvent(logger, level, __FILE__, __LINE__, 0, Framework::GetThreadId(), Framework::Multithread::GetName(), Framework::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)
#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, Framework::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, Framework::LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, Framework::LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, Framework::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, Framework::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT() Framework::loggerMgr::GetInstance()->getRoot()
#define LOG_NAME(name) Framework::loggerMgr::GetInstance()->getLogger(name)

namespace Framework {
    class Logger;
    class LoggerManager;

    class LogLevel {
    public:
        // 定义日志级别的枚举类型Level
        enum Level {
            DEBUG = 1,  // 调试级别
            INFO = 2,   // 信息级别
            WARN = 3,   // 警告级别
            ERROR = 4,  // 错误级别
            FATAL = 5   // 致命错误级别
        };

        static const char* toString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string& str);
    };

    // 日志事件
    class LogEvent {
    public:
        // 定义LogEvent智能指针类型别名ptr
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t m_line, uint32_t elapse, uint32_t thread_id, const std::string& thread_name, uint32_t fiber_id, uint64_t time);

        const char* getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        const std::string& getThreadName() const { return m_threadName; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_content.str(); }

        // 流式
        std::stringstream& getSS() { return m_content; }
        // format方法
		void format(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);

            // 先计算需要的缓冲区大小（使用 _vscprintf）
            int size = _vscprintf(fmt, args) + 1;  // +1 用于 '\0'
            va_end(args);

            // 动态分配缓冲区
            char* buffer = (char*)malloc(size);

            // 重新获取可变参数并安全格式化
            va_start(args, fmt);
            vsprintf_s(buffer, size, fmt, args);  // 安全版本
            va_end(args);

			this->getSS() << buffer;
		}
        #ifndef _WIN32
        void format(const char* fmt, va_list al) {
            char* buf = nullptr;
            // 使用vasprintf函数将格式化的字符串写入buf，并获取其长度
            int len = Framework::vasprintf(&buf, fmt, al);
            if (len != -1) {
                // 将格式化后的字符串存储到成员变量m_ss中
                //m_ss << std::string(buf, len);
                // 释放动态分配的内存
                free(buf);
            }
        }
        #endif 

        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLogLevel() const { return m_level; }
    private:
        // 记录日志的文件名，初始化为空指针
        const char* m_file = nullptr;
        // 记录日志的行号，初始化为0
        int32_t m_line = 0;
        //程序启动开始到现在的毫秒数
        int32_t m_elapse = 0;
        // 线程ID，初始化为0
        uint32_t m_threadId = 0;
        std::string m_threadName;
        // 协程ID，初始化为0
        uint32_t m_fiberId = 0;
        // 日志记录的时间戳，初始化为0
        uint64_t m_time = 0;
        // 日志内容
        std::stringstream m_content;

        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };

    class LogEventWrap {
    public:
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();
        std::stringstream& getSS();
        std::shared_ptr<LogEvent> getEvent() { return m_event; }
    private:
        LogEvent::ptr m_event;
    };

    //日志格式器
    class LogFormatter {
    public:
        // 定义一个指向LogFormatter的智能指针类型
        typedef std::shared_ptr<LogFormatter> ptr;
        // 构造函数，接受一个字符串模式参数
        LogFormatter(const std::string& pattern);
        // 格式化日志事件的函数，接受一个LogEvent的智能指针参数，返回格式化后的字符串
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    public:
        // 内部类FormatItem，用于表示格式化项
        class FormatItem {
        public:
            // 定义一个指向FormatItem的智能指针类型
            typedef std::shared_ptr<FormatItem> ptr;
            // 构造函数
            //FormatItem(const std::string& fmt = "") {};
            // 虚析构函数，确保派生类正确析构
            virtual ~FormatItem() {}
            // 纯虚函数，用于格式化日志事件，接受一个LogEvent的智能指针参数，返回格式化后的字符串
            virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();

        bool isError() const { return m_error; }
        const std::string getPattern() const { return m_pattern; }
    private:
        // 存储日志格式模式的字符串
        std::string m_pattern;
        // 存储格式化项的智能指针向量
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    // 日志输出地相关类
    class LogAppender {
    friend class Logger;
    public:
        // 定义指向LogAppender对象的智能指针类型
        typedef std::shared_ptr<LogAppender> ptr;
        // 虚析构函数，用于在派生类析构时正确释放资源
        virtual ~LogAppender() {}
        // 记录日志的函数，接受日志级别和日志事件指针作为参数
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        virtual std::string toYamlString() = 0;

        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }
    protected:
        // 日志级别
        LogLevel::Level m_level = LogLevel::DEBUG;
        bool m_hasFormatter = false;
        LogFormatter::ptr m_formatter;

        Spinlock m_mutex;
    };

    // 日志器类定义
    class Logger : public std::enable_shared_from_this<Logger>{
    friend class LoggerManager;
    public:
        // 定义一个指向Logger的智能指针类型别名
        typedef std::shared_ptr<Logger> ptr;

        // 构造函数，接受日志名称参数，默认值为"root"
        Logger(const std::string& name = "root");

        // 记录日志的方法，接受日志级别和日志事件指针作为参数
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppenders();

        LogLevel::Level getLevel() const {return m_level;}
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string& getName() const { return m_name; }

        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string& val);
        LogFormatter::ptr getFormatter() { Spinlock::Lock lock(m_mutex); return m_formatter; }

        std::string toYamlString();
    private:
        // 日志名称成员变量
        std::string m_name;
        // 日志级别成员变量
        LogLevel::Level m_level;
        // Appender集合成员变量，用于存储多个Appender指针
        std::list<LogAppender::ptr> m_appenders;
        LogFormatter::ptr m_formatter;
        Logger::ptr m_root;

        Spinlock m_mutex;
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender {
    public:
        // 定义一个指向StdoutLogAppender的智能指针类型别名
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        // 重写父类的log方法，用于将日志事件输出到控制台，level表示日志级别，event表示日志事件
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;
    };

    // 定义输出到文件的Appender
    class FileLogAppender : public LogAppender {
    public:
        // 定义一个指向FileLogAppender的智能指针类型别名
        typedef std::shared_ptr<FileLogAppender> ptr;
        // 构造函数，接受一个文件名参数，用于指定日志输出的文件
        FileLogAppender(const std::string& filename);
        // 重写父类的log方法，用于将日志事件输出到文件，level表示日志级别，event表示日志事件
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;

        bool reopen();
    private:
        // 日志记录器的名称
        std::string m_filename;
        // 用于写入日志的文件流对象
        std::ofstream m_filestream;
        uint64_t m_reopenTime;
    };
}

namespace Framework {
    class LoggerManager {
    public:
        LoggerManager();
        Logger::ptr getLogger(const std::string& name);
        Logger::ptr getRoot() const { return m_root; }
        std::string toYamlString();

        void init();
    private:
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;

        Spinlock m_mutex;
    };

    typedef Framework::Singleton<LoggerManager> loggerMgr;
}

#endif