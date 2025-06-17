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
#undef ERROR  // ȡ���궨��
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
        // ������־�����ö������Level
        enum Level {
            DEBUG = 1,  // ���Լ���
            INFO = 2,   // ��Ϣ����
            WARN = 3,   // ���漶��
            ERROR = 4,  // ���󼶱�
            FATAL = 5   // �������󼶱�
        };

        static const char* toString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string& str);
    };

    // ��־�¼�
    class LogEvent {
    public:
        // ����LogEvent����ָ�����ͱ���ptr
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

        // ��ʽ
        std::stringstream& getSS() { return m_content; }
        // format����
		void format(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);

            // �ȼ�����Ҫ�Ļ�������С��ʹ�� _vscprintf��
            int size = _vscprintf(fmt, args) + 1;  // +1 ���� '\0'
            va_end(args);

            // ��̬���仺����
            char* buffer = (char*)malloc(size);

            // ���»�ȡ�ɱ��������ȫ��ʽ��
            va_start(args, fmt);
            vsprintf_s(buffer, size, fmt, args);  // ��ȫ�汾
            va_end(args);

			this->getSS() << buffer;
		}
        #ifndef _WIN32
        void format(const char* fmt, va_list al) {
            char* buf = nullptr;
            // ʹ��vasprintf��������ʽ�����ַ���д��buf������ȡ�䳤��
            int len = Framework::vasprintf(&buf, fmt, al);
            if (len != -1) {
                // ����ʽ������ַ����洢����Ա����m_ss��
                //m_ss << std::string(buf, len);
                // �ͷŶ�̬������ڴ�
                free(buf);
            }
        }
        #endif 

        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLogLevel() const { return m_level; }
    private:
        // ��¼��־���ļ�������ʼ��Ϊ��ָ��
        const char* m_file = nullptr;
        // ��¼��־���кţ���ʼ��Ϊ0
        int32_t m_line = 0;
        //����������ʼ�����ڵĺ�����
        int32_t m_elapse = 0;
        // �߳�ID����ʼ��Ϊ0
        uint32_t m_threadId = 0;
        std::string m_threadName;
        // Э��ID����ʼ��Ϊ0
        uint32_t m_fiberId = 0;
        // ��־��¼��ʱ�������ʼ��Ϊ0
        uint64_t m_time = 0;
        // ��־����
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

    //��־��ʽ��
    class LogFormatter {
    public:
        // ����һ��ָ��LogFormatter������ָ������
        typedef std::shared_ptr<LogFormatter> ptr;
        // ���캯��������һ���ַ���ģʽ����
        LogFormatter(const std::string& pattern);
        // ��ʽ����־�¼��ĺ���������һ��LogEvent������ָ����������ظ�ʽ������ַ���
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    public:
        // �ڲ���FormatItem�����ڱ�ʾ��ʽ����
        class FormatItem {
        public:
            // ����һ��ָ��FormatItem������ָ������
            typedef std::shared_ptr<FormatItem> ptr;
            // ���캯��
            //FormatItem(const std::string& fmt = "") {};
            // ������������ȷ����������ȷ����
            virtual ~FormatItem() {}
            // ���麯�������ڸ�ʽ����־�¼�������һ��LogEvent������ָ����������ظ�ʽ������ַ���
            virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();

        bool isError() const { return m_error; }
        const std::string getPattern() const { return m_pattern; }
    private:
        // �洢��־��ʽģʽ���ַ���
        std::string m_pattern;
        // �洢��ʽ���������ָ������
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    // ��־����������
    class LogAppender {
    friend class Logger;
    public:
        // ����ָ��LogAppender���������ָ������
        typedef std::shared_ptr<LogAppender> ptr;
        // ����������������������������ʱ��ȷ�ͷ���Դ
        virtual ~LogAppender() {}
        // ��¼��־�ĺ�����������־�������־�¼�ָ����Ϊ����
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        virtual std::string toYamlString() = 0;

        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }
    protected:
        // ��־����
        LogLevel::Level m_level = LogLevel::DEBUG;
        bool m_hasFormatter = false;
        LogFormatter::ptr m_formatter;

        Spinlock m_mutex;
    };

    // ��־���ඨ��
    class Logger : public std::enable_shared_from_this<Logger>{
    friend class LoggerManager;
    public:
        // ����һ��ָ��Logger������ָ�����ͱ���
        typedef std::shared_ptr<Logger> ptr;

        // ���캯����������־���Ʋ�����Ĭ��ֵΪ"root"
        Logger(const std::string& name = "root");

        // ��¼��־�ķ�����������־�������־�¼�ָ����Ϊ����
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
        // ��־���Ƴ�Ա����
        std::string m_name;
        // ��־�����Ա����
        LogLevel::Level m_level;
        // Appender���ϳ�Ա���������ڴ洢���Appenderָ��
        std::list<LogAppender::ptr> m_appenders;
        LogFormatter::ptr m_formatter;
        Logger::ptr m_root;

        Spinlock m_mutex;
    };

    // ���������̨��Appender
    class StdoutLogAppender : public LogAppender {
    public:
        // ����һ��ָ��StdoutLogAppender������ָ�����ͱ���
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        // ��д�����log���������ڽ���־�¼����������̨��level��ʾ��־����event��ʾ��־�¼�
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;
    };

    // ����������ļ���Appender
    class FileLogAppender : public LogAppender {
    public:
        // ����һ��ָ��FileLogAppender������ָ�����ͱ���
        typedef std::shared_ptr<FileLogAppender> ptr;
        // ���캯��������һ���ļ�������������ָ����־������ļ�
        FileLogAppender(const std::string& filename);
        // ��д�����log���������ڽ���־�¼�������ļ���level��ʾ��־����event��ʾ��־�¼�
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;

        bool reopen();
    private:
        // ��־��¼��������
        std::string m_filename;
        // ����д����־���ļ�������
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