#include "log.h"

namespace Framework {
    // ---LogLevel---
    const char* LogLevel::toString(LogLevel::Level level) {
        switch (level) {
        #define XX(name) \
            case LogLevel::name: \
                return #name; \
                break;

                XX(DEBUG);
                XX(INFO);
                XX(WARN);
                XX(ERROR);
                XX(FATAL);
        #undef XX
            default:
                return "UNKNOWN";
        }
        return "UNKNOWN";
    }

    // ---Logger---
    // Logger类的构造函数，接受一个字符串参数name，用于初始化日志记录器的名称
    Logger::Logger(const std::string& name)
        : m_name(name) {
    }

    // 向日志记录器中添加一个日志输出器（appender）
    void Logger::addAppender(LogAppender::ptr appender) {
        m_appenders.push_back(appender);
    }

    // 从日志记录器中删除指定的日志输出器（appender）
    void Logger::delAppender(LogAppender::ptr appender) {
        for (auto it = m_appenders.begin();
            it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    // 记录日志的方法，根据日志级别判断是否记录，若满足条件则遍历所有输出器进行日志输出
    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            for (auto& i : m_appenders) {
                i->log(level, event);
            }
        }
    }

    // 调试级别的日志记录方法，调用log方法并指定级别为DEBUG
    void Logger::debug(LogEvent::ptr event) {
        debug(LogLevel::DEBUG, event);
    }

    // 信息级别的日志记录方法，调用log方法并指定级别为INFO
    void Logger::info(LogEvent::ptr event) {
        debug(LogLevel::INFO, event);
    }

    // 警告级别的日志记录方法，调用log方法并指定级别为WARN
    void Logger::warn(LogEvent::ptr event) {
        debug(LogLevel::WARN, event);
    }

    // 错误级别的日志记录方法，调用log方法并指定级别为ERROR
    void Logger::error(LogEvent::ptr event) {
        debug(LogLevel::ERROR, event);
    }

    // 致命错误级别的日志记录方法，调用log方法并指定级别为FATAL
    void Logger::fatal(LogEvent::ptr event) {
        debug(LogLevel::FATAL, event);
    }

    // ---LogAppender---
    // FileLogAppender类的构造函数，接受一个文件名参数
    // 用于初始化成员变量m_filename，该变量存储日志要写入的文件名
    FileLogAppender::FileLogAppender(const std::string& filename)
        : m_filename(filename) {
    }

    // FileLogAppender类的log方法，用于将日志事件写入文件
    // level表示日志的级别
    // event是指向日志事件的智能指针
    void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
        // 判断当前日志级别是否大于等于设定的日志级别
        if (level >= m_level) {
            // 如果满足条件，将格式化后的日志事件写入到文件流m_filestream中
            m_filestream << m_formatter->format(level, event);
        }
    }

    // FileLogAppender类的reopen函数，用于重新打开日志文件流
    // 返回值：如果文件流成功打开（即文件流对象有效），返回true；否则返回false
    bool FileLogAppender::reopen() {
        // 判断文件流对象m_filestream是否存在（不为nullptr）
        if (m_filestream) {
            // 关闭当前的文件流
            m_filestream.close();
        }
        // 尝试打开指定文件名的文件流
        m_filestream.open(m_filename);
        // 返回文件流是否成功打开的状态，!m_filestream为false表示打开失败，true表示打开成功
        return!m_filestream;
    }

    // StdoutLogAppender类的log函数，用于将日志事件输出到标准输出流
    // level：日志级别
    // event：指向日志事件对象的智能指针
    void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
        // 判断当前日志级别是否大于等于设置的日志级别
        if (level >= m_level) {
            // 将格式化后的日志事件输出到标准输出流std::cout
            std::cout << m_formatter->format(level, event);
        }
    }

    // ---LogFormatter---
    LogFormatter::LogFormatter(const std::string& pattern)
        : m_pattern(pattern) {
    }

    std::string LogFormatter::format(LogLevel::Level level, LogEvent::ptr event) {
        std::stringstream ss;
        for (auto& i : m_items) {
            i->format(ss, level, event);
        }
        return ss.str();
    }

    void LogFormatter::init() {
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); ++i) {
            // 如果当前字符不是 '%', 直接将其追加到 nstr 中
            if (m_pattern[i] != '%') {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            // 如果当前字符是 '%' 且下一个字符也是 '%', 说明是转义的 '%', 
            // 直接将 '%' 追加到 nstr 中然后跳过下一个字符的检查
            if ((i + 1) < m_pattern.size()) {
                if (m_pattern[i + 1] == '%') {
                    nstr.append(1, '%');
                    continue;
                }
            }
            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;
            std::string str;
            std::string fmt;
            while (n < m_pattern.size()) {
                // 如果遇到空格，停止解析当前格式相关内容
                if (isspace(m_pattern[n])) {
                    break;
                }
                // 当还未开始解析格式标识时
                if (fmt_status == 0) {
                    // 如果遇到 '('，表示格式标识开始
                    if (m_pattern[n] == '(') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; // 标记进入格式解析状态
                        fmt_begin = n; // 记录格式标识开始的索引
                        ++n;
                        continue;
                    }
                }
                // 当已经开始解析格式标识时
                if (fmt_status == 1) {
                    // 如果遇到 ')'，表示格式标识结束
                    if (m_pattern[n] == ')') {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 2; // 标记格式解析完成
                        break;
                    }
                }
            }

            if (fmt_status == 0) {
                if (!nstr.empty()) {
                    // 如果nstr不为空，将nstr和一个空字符串组成的pair（第三个参数0可能表示某种状态或标识）添加到vec向量中
                    vec.push_back(std::make_tuple(nstr, "", 0));
                }
                // 从m_pattern字符串中截取从索引i + 1开始，长度为n - i - 1的子字符串，赋值给str
                str = m_pattern.substr(i + 1, n - i - 1);
                // 将str和fmt组成的tuple（第三个参数1可能表示某种状态或标识）添加到vec向量中
                vec.push_back(std::make_tuple(str, fmt, 1));
                // 将索引i设置为n，可能用于后续循环控制或表示处理结束等
                i = n;
            }
            else if (fmt_status == 1) {
                // 如果fmt_status等于1，输出格式解析错误信息，包括模式字符串m_pattern以及当前位置i处的子字符串
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                // 将一个包含"<pattern_error>"和fmt的tuple（第三个参数0可能表示某种状态或标识）添加到vec向量中
                vec.push_back(std::make_tuple("<pattern_error>", fmt, 0));
            }
            else if (fmt_status == 2) {
                if (!nstr.empty()) {
                    // 如果nstr不为空，将nstr和一个空字符串组成的pair（第三个参数0可能表示某种状态或标识）添加到vec向量中
                    vec.push_back(std::make_tuple(nstr, "", 0));
                }
                // 将str和fmt组成的tuple（第三个参数1可能表示某种状态或标识）添加到vec向量中
                vec.push_back(std::make_tuple(str, fmt, 1));
                // 将索引i设置为n，可能用于后续循环控制或表示处理结束等
                i = n;
            }
        }

        if (!nstr.empty()) {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }
    }

    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        void format(std::ostream& os, LogLevel::Level level, LogEvent::ptr event) override {
            os << LogLevel::toString(level);
        }
    };
}