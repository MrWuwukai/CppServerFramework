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

namespace Framework {

    // 日志事件
    class LogEvent {
    public:
        // 定义LogEvent智能指针类型别名ptr
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent();

        const char* getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        const std::string& getContent() const { return m_content; }
    private:
        // 记录日志的文件名，初始化为空指针
        const char* m_file = nullptr;
        // 记录日志的行号，初始化为0
        int32_t m_line = 0;
        //程序启动开始到现在的毫秒数
        int32_t m_elapse = 0;
        // 线程ID，初始化为0
        uint32_t m_threadId = 0;
        // 协程ID，初始化为0
        uint32_t m_fiberId = 0;
        // 日志记录的时间戳，初始化为0
        uint64_t m_time;
        // 日志内容
        std::string m_content;
    };

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
    };

    //日志格式器
    class LogFormatter {
    public:
        // 定义一个指向LogFormatter的智能指针类型
        typedef std::shared_ptr<LogFormatter> ptr;
        // 构造函数，接受一个字符串模式参数
        LogFormatter(const std::string& pattern);
        // 格式化日志事件的函数，接受一个LogEvent的智能指针参数，返回格式化后的字符串
        std::string format(LogLevel::Level level, LogEvent::ptr event);
    public:
        // 内部类FormatItem，用于表示格式化项
        class FormatItem {
        public:
            // 定义一个指向FormatItem的智能指针类型
            typedef std::shared_ptr<FormatItem> ptr;
            // 虚析构函数，确保派生类正确析构
            virtual ~FormatItem() {}
            // 纯虚函数，用于格式化日志事件，接受一个LogEvent的智能指针参数，返回格式化后的字符串
            virtual void format(std::ostream& os, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();
    private:
        // 存储日志格式模式的字符串
        std::string m_pattern;
        // 存储格式化项的智能指针向量
        std::vector<FormatItem::ptr> m_items;
    };

    // 日志输出地相关类
    class LogAppender {
    public:
        // 定义指向LogAppender对象的智能指针类型
        typedef std::shared_ptr<LogAppender> ptr;
        // 虚析构函数，用于在派生类析构时正确释放资源
        virtual ~LogAppender() {}
        // 记录日志的函数，接受日志级别和日志事件指针作为参数
        virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }
    protected:
        // 日志级别
        LogLevel::Level m_level;
        LogFormatter::ptr m_formatter;
    };

    // 日志器类定义
    class Logger {
    public:
        // 定义一个指向Logger的智能指针类型别名
        typedef std::shared_ptr<Logger> ptr;

        // 构造函数，接受日志名称参数，默认值为"root"
        Logger(const std::string& name = "root");

        // 记录日志的方法，接受日志级别和日志事件指针作为参数
        void log(LogLevel::Level level, LogEvent::ptr event);

        // 记录调试级别日志的方法，接受日志事件指针作为参数
        void debug(LogEvent::ptr event);

        // 记录信息级别日志的方法，接受日志事件指针作为参数
        void info(LogEvent::ptr event);

        // 记录警告级别日志的方法，接受日志事件指针作为参数
        void warn(LogEvent::ptr event);

        // 记录错误级别日志的方法，接受日志事件指针作为参数
        void error(LogEvent::ptr event);

        // 记录致命错误级别日志的方法，接受日志事件指针作为参数
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);

        LogLevel::Level getLevel() const {return m_level;}
        void setLevel(LogLevel::Level val) { m_level = val; }

    private:
        // 日志名称成员变量
        std::string m_name;
        // 日志级别成员变量
        LogLevel::Level m_level;
        // Appender集合成员变量，用于存储多个Appender指针
        std::list<LogAppender::ptr> m_appenders;
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender {
    public:
        // 定义一个指向StdoutLogAppender的智能指针类型别名
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        // 重写父类的log方法，用于将日志事件输出到控制台，level表示日志级别，event表示日志事件
        void log(LogLevel::Level level, LogEvent::ptr event) override;
    };

    // 定义输出到文件的Appender
    class FileLogAppender : public LogAppender {
    public:
        // 定义一个指向FileLogAppender的智能指针类型别名
        typedef std::shared_ptr<FileLogAppender> ptr;
        // 构造函数，接受一个文件名参数，用于指定日志输出的文件
        FileLogAppender(const std::string& filename);
        // 重写父类的log方法，用于将日志事件输出到文件，level表示日志级别，event表示日志事件
        void log(LogLevel::Level level, LogEvent::ptr event) override;

        bool reopen();
    private:
        // 日志记录器的名称
        std::string m_filename;
        // 用于写入日志的文件流对象
        std::ofstream m_filestream;
    };
}

#endif