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

#ifdef _WIN32
#include <ctime>
#include <cstdio> 
#else
#include <time.h>
#include <string.h>
#endif

//#define LOG_LEVEL(logger, level) if(logger->getLevel() <= level) LogEventWrap(LogEvent::ptr(new LogEvent(__FILE__, __LINE__, 0, Framework::GetThreadId(), Framework::GetFiberId(), time(0))))->getSS()
#define LOG_LEVEL(logger, level) Framework::LogEventWrap(Framework::LogEvent::ptr(new Framework::LogEvent(logger, level, __FILE__, __LINE__, 0, Framework::GetThreadId(), Framework::GetFiberId(), time(0)))).getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger, Framework::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, Framework::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, Framework::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, Framework::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, Framework::LogLevel::FATAL)

namespace Framework {
    class Logger;

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
    };

    // ��־�¼�
    class LogEvent {
    public:
        // ����LogEvent����ָ�����ͱ���ptr
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t m_line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);

        const char* getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        std::string getContent() const { return m_content.str(); }

        std::stringstream& getSS() { return m_content; }
        void format(const char* fmt, ...);

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
    private:
        // �洢��־��ʽģʽ���ַ���
        std::string m_pattern;
        // �洢��ʽ���������ָ������
        std::vector<FormatItem::ptr> m_items;
    };

    // ��־����������
    class LogAppender {
    public:
        // ����ָ��LogAppender���������ָ������
        typedef std::shared_ptr<LogAppender> ptr;
        // ����������������������������ʱ��ȷ�ͷ���Դ
        virtual ~LogAppender() {}
        // ��¼��־�ĺ�����������־�������־�¼�ָ����Ϊ����
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }
    protected:
        // ��־����
        LogLevel::Level m_level = LogLevel::INFO;
        LogFormatter::ptr m_formatter;
    };

    // ��־���ඨ��
    class Logger : public std::enable_shared_from_this<Logger>{
    public:
        // ����һ��ָ��Logger������ָ�����ͱ���
        typedef std::shared_ptr<Logger> ptr;

        // ���캯����������־���Ʋ�����Ĭ��ֵΪ"root"
        Logger(const std::string& name = "root");

        // ��¼��־�ķ�����������־�������־�¼�ָ����Ϊ����
        void log(LogLevel::Level level, LogEvent::ptr event);

        // ��¼���Լ�����־�ķ�����������־�¼�ָ����Ϊ����
        void debug(LogEvent::ptr event);

        // ��¼��Ϣ������־�ķ�����������־�¼�ָ����Ϊ����
        void info(LogEvent::ptr event);

        // ��¼���漶����־�ķ�����������־�¼�ָ����Ϊ����
        void warn(LogEvent::ptr event);

        // ��¼���󼶱���־�ķ�����������־�¼�ָ����Ϊ����
        void error(LogEvent::ptr event);

        // ��¼�������󼶱���־�ķ�����������־�¼�ָ����Ϊ����
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);

        LogLevel::Level getLevel() const {return m_level;}
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string& getName() const { return m_name; }

    private:
        // ��־���Ƴ�Ա����
        std::string m_name;
        // ��־�����Ա����
        LogLevel::Level m_level;
        // Appender���ϳ�Ա���������ڴ洢���Appenderָ��
        std::list<LogAppender::ptr> m_appenders;
        LogFormatter::ptr m_formatter;
    };

    // ���������̨��Appender
    class StdoutLogAppender : public LogAppender {
    public:
        // ����һ��ָ��StdoutLogAppender������ָ�����ͱ���
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        // ��д�����log���������ڽ���־�¼����������̨��level��ʾ��־����event��ʾ��־�¼�
        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
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

        bool reopen();
    private:
        // ��־��¼��������
        std::string m_filename;
        // ����д����־���ļ�������
        std::ofstream m_filestream;
    };
}

#endif