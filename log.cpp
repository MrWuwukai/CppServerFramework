#include "log.h"
#include "config.h"

namespace Framework {
    // ---LogEvent---
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t m_line, uint32_t elapse, uint32_t thread_id, const std::string& thread_name, uint32_t fiber_id, uint64_t time)
        : m_file(file)
        , m_line(m_line)
        , m_elapse(elapse)
        , m_threadId(thread_id)
        , m_threadName(thread_name)
        , m_fiberId(fiber_id)
        , m_time(time) 
        , m_logger(logger)
        , m_level(level) {

    }

    // ---LogEventWrap---
    LogEventWrap::LogEventWrap(LogEvent::ptr e)
        :m_event(e) {
    }

    LogEventWrap::~LogEventWrap() {
        m_event->getLogger()->log(m_event->getLogLevel(), m_event);
    }

    std::stringstream& LogEventWrap::getSS() {
        return m_event->getSS();
    }

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
    LogLevel::Level LogLevel::FromString(const std::string& str) {
        std::string upperStr = str;
        std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(),
            [](unsigned char c) { return std::toupper(c); });
    #define XX(name) if(upperStr == #name){return LogLevel::name;}
		XX(DEBUG);
		XX(INFO);
		XX(WARN);
		XX(ERROR);
		XX(FATAL);
        return LogLevel::DEBUG;
    #undef XX
    }

    // ---Logger---
    // Logger类的构造函数，接受一个字符串参数name，用于初始化日志记录器的名称
    Logger::Logger(const std::string& name)
        : m_name(name), m_level(LogLevel::INFO) {
        //m_formatter.reset(new LogFormatter("%d [%p] <%f:%l> %m %n"));
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S} %T%t %T%N %T%F %T[%p]%T[%c] %T%f:%l %T%m%n"));
    }

    // 向日志记录器中添加一个日志输出器（appender）
    void Logger::addAppender(LogAppender::ptr appender) {
        Spinlock::Lock lock(m_mutex);
        if (!appender->getFormatter()){
            Spinlock::Lock innerlock(appender->m_mutex);
            appender->m_formatter = m_formatter;
        }
        m_appenders.push_back(appender);
    }

    // 从日志记录器中删除指定的日志输出器（appender）
    void Logger::delAppender(LogAppender::ptr appender) {
        Spinlock::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders() {
        m_appenders.clear();
    }

    void Logger::setFormatter(LogFormatter::ptr val) {
        Spinlock::Lock lock(m_mutex);
        m_formatter = val;

        for (auto& i : m_appenders) {
            Spinlock::Lock innerlock(i->m_mutex);
            if (!i->m_hasFormatter) {
                i->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string& val) {
        LogFormatter::ptr new_val(new LogFormatter(val));
        if (new_val->isError()) {
            std::cout << "Logger setFormatter name=" << m_name
                << " value=" << val << " invalid formatter"
                << std::endl;
            return;
        }
        //m_formatter = new_val;
        setFormatter(new_val);
    }

    // 记录日志的方法，根据日志级别判断是否记录，若满足条件则遍历所有输出器进行日志输出
    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            auto self = shared_from_this();
            Spinlock::Lock lock(m_mutex);
            if (!m_appenders.empty()) {
                for (auto& i : m_appenders) {
                    //Spinlock::Lock innerlock(i->m_mutex);
                    i->log(self, level, event);
                }
            }
            else if (m_root) {
                m_root->log(level, event);
            }
        }
    }

    // 调试级别的日志记录方法，调用log方法并指定级别为DEBUG
    void Logger::debug(LogEvent::ptr event) {
        log(LogLevel::DEBUG, event);
    }

    // 信息级别的日志记录方法，调用log方法并指定级别为INFO
    void Logger::info(LogEvent::ptr event) {
        log(LogLevel::INFO, event);
    }

    // 警告级别的日志记录方法，调用log方法并指定级别为WARN
    void Logger::warn(LogEvent::ptr event) {
        log(LogLevel::WARN, event);
    }

    // 错误级别的日志记录方法，调用log方法并指定级别为ERROR
    void Logger::error(LogEvent::ptr event) {
        log(LogLevel::ERROR, event);
    }

    // 致命错误级别的日志记录方法，调用log方法并指定级别为FATAL
    void Logger::fatal(LogEvent::ptr event) {
        log(LogLevel::FATAL, event);
    }

    std::string Logger::toYamlString() {
        Spinlock::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        node["level"] = LogLevel::toString(m_level);
        if (m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }

        for (auto& i : m_appenders) {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
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
    void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        // 判断当前日志级别是否大于等于设定的日志级别
        if (level >= m_level) {
            // 这里检测到中途文件是否突然被删除，当被删除时立刻重新打开文件
            uint64_t now = time(0);
            if (now != m_reopenTime) {
                reopen();
                m_reopenTime = now;
            }
            Spinlock::Lock lock(m_mutex);
            // 如果满足条件，将格式化后的日志事件写入到文件流m_filestream中  
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

    // FileLogAppender类的reopen函数，用于重新打开日志文件流
    // 返回值：如果文件流成功打开（即文件流对象有效），返回true；否则返回false
    bool FileLogAppender::reopen() {
        Spinlock::Lock lock(m_mutex);
        // 判断文件流对象m_filestream是否存在（不为nullptr）
        if (m_filestream) {
            // 关闭当前的文件流
            m_filestream.close();
        }
        // 尝试打开指定文件名的文件流
        m_filestream.open(m_filename);
        // 返回文件流是否成功打开的状态，!m_filestream为false表示打开失败，true表示打开成功
        return m_filestream.is_open();
    }

    std::string FileLogAppender::toYamlString() {
        Spinlock::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        node["level"] = LogLevel::toString(m_level);
        if (m_hasFormatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void LogAppender::setFormatter(LogFormatter::ptr val) {
        Spinlock::Lock lock(m_mutex);
        m_formatter = val;
        if (m_formatter) {
            m_hasFormatter = true;
        }
        else {
            m_hasFormatter = false;
        }
    }

    LogFormatter::ptr LogAppender::getFormatter() {
        Spinlock::Lock lock(m_mutex);
        return m_formatter;
    }

    // StdoutLogAppender类的log函数，用于将日志事件输出到标准输出流
    // level：日志级别
    // event：指向日志事件对象的智能指针
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        // 判断当前日志级别是否大于等于设置的日志级别
        if (level >= m_level) {
            Spinlock::Lock lock(m_mutex);
            // 将格式化后的日志事件输出到标准输出流std::cout
            std::cout << m_formatter->format(logger, level, event);
        }
    }
    std::string StdoutLogAppender::toYamlString() {
        Spinlock::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        node["level"] = LogLevel::toString(m_level);
        if (m_hasFormatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    // ---LogFormatter---
    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        MessageFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        LevelFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << LogLevel::toString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem {
    public:
        ElapseFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem {
    public:
        NameFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadIdFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadNameFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadName();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem {
    public:
        FiberIdFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem {
    public:
        DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
            : m_format(format) {
            if (m_format.empty()){
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            //os << event->getTime();
            struct tm tm;
            time_t time = event->getTime();

            #ifdef _WIN32
            _localtime64_s(&tm, &time);
            #else
            localtime_r(&time, &tm);
            #endif

            char buf[64];

            #ifdef _WIN32
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            #else
            strptime(buf, sizeof(tm), m_format.c_str(), &tm);
            #endif
            os << buf;
        }
    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem {
    public:
        FilenameFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem {
    public:
        LineFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem {
    public:
        NewLineFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem {
    public:
        StringFormatItem(const std::string& str) : m_string(str) {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << m_string;
        }
    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem {
    public:
        TabFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << "\t";
        }
    private:
        std::string m_string;
    };

    LogFormatter::LogFormatter(const std::string& pattern)
        : m_pattern(pattern) {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        std::stringstream ss;
        for (auto& i : m_items) {
            i->format(ss, logger, level, event);
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
                if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')) {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                // 当还未开始解析格式标识时
                if (fmt_status == 0) {
                    // 如果遇到 '('，表示格式标识开始
                    if (m_pattern[n] == '{') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; // 标记进入格式解析状态
                        fmt_begin = n; // 记录格式标识开始的索引
                        ++n;
                        continue;
                    }
                }
                // 当已经开始解析格式标识时
                else if (fmt_status == 1) {
                    // 如果遇到 ')'，表示格式标识结束
                    if (m_pattern[n] == '}') {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0; // 标记格式解析完成
                        ++n;
                        break;
                    }
                }
                ++n;
                
                if (n == m_pattern.size()) {
                    if (str.empty()) {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if (fmt_status == 0) {
                if (!nstr.empty()) {
                    // 如果nstr不为空，将nstr和一个空字符串组成的pair（第三个参数0可能表示某种状态或标识）添加到vec向量中
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                // 将str和fmt组成的tuple（第三个参数1可能表示某种状态或标识）添加到vec向量中
                vec.push_back(std::make_tuple(str, fmt, 1));
                // 将索引i设置为n，可能用于后续循环控制或表示处理结束等
                i = n - 1;
            }
            else if (fmt_status == 1) {
                // 如果fmt_status等于1，输出格式解析错误信息，包括模式字符串m_pattern以及当前位置i处的子字符串
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                m_error = true;
                // 将一个包含"<pattern_error>"和fmt的tuple（第三个参数0可能表示某种状态或标识）添加到vec向量中
                vec.push_back(std::make_tuple("<pattern_error>", fmt, 0));
            }
            //else if (fmt_status == 2) {
            //    if (!nstr.empty()) {
            //        // 如果nstr不为空，将nstr和一个空字符串组成的pair（第三个参数0可能表示某种状态或标识）添加到vec向量中
            //        vec.push_back(std::make_tuple(nstr, "", 0));
            //        nstr.clear();
            //    }
            //    // 将str和fmt组成的tuple（第三个参数1可能表示某种状态或标识）添加到vec向量中
            //    vec.push_back(std::make_tuple(str, fmt, 1));
            //    // 将索引i设置为n，可能用于后续循环控制或表示处理结束等
            //    i = n - 1;
            //}
        }

        if (!nstr.empty()) {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
            // # 是 C/C++ 预处理器的字符串化运算符，它的作用是将宏参数转换为字符串字面量。
            // #str 会将传入的 str 参数（如 "m"、"p" 等）转换成字符串字面量（如 "m"、"p"）。
            #define XX(str, C) \
                {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); }}
                XX(m, MessageFormatItem),
                XX(p, LevelFormatItem),
                XX(r, ElapseFormatItem),
                XX(c, NameFormatItem),
                XX(t, ThreadIdFormatItem),
                XX(n, NewLineFormatItem),
                XX(d, DateTimeFormatItem),
                XX(f, FilenameFormatItem),
                XX(l, LineFormatItem),
                XX(T, TabFormatItem),
                XX(F, FiberIdFormatItem),
                XX(N, ThreadNameFormatItem)
            #undef XX
        };

        for (auto& i : vec) {
            // 判断元组i的第三个元素是否为0
            if (std::get<2>(i) == 0) {
                // 如果是0，创建一个StringFormatItem对象，并将其指针包装在FormatItem::ptr中，
                // 然后添加到m_items容器中。这里假设StringFormatItem是一个自定义类，
                // 它的构造函数接受元组i的第一个元素作为参数
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else {
                // 如果元组i的第三个元素不为0，在s_format_items这个容器（假设是一个map之类的关联容器）中查找键为元组i的第一个元素
                auto it = s_format_items.find(std::get<0>(i));
                // 如果没有找到对应的键
                if (it == s_format_items.end()) {
                    // 创建一个格式化错误信息的StringFormatItem对象，并将其指针包装在FormatItem::ptr中，
                    // 添加到m_items容器中。格式化信息包含原始查找的键
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<error_format " + std::get<0>(i) + ">")));
                    m_error = true;
                }
                else {
                    // 如果找到了对应的键，调用找到的值的第二个元素（假设是一个可调用对象，比如函数指针或函数对象）
                    // 并传入元组i的第二个元素作为参数，将返回值包装在FormatItem::ptr中，添加到m_items容器中
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            // test
            // std::cout << std::get<0>(i) << " - " << std::get<1>(i) << " - " << std::get<2>(i) << std::endl;
        }
    }

    
}

namespace Framework {
    LoggerManager::LoggerManager() {
        m_root.reset(new Logger);
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->m_name] = m_root;
        init();
    }

    Logger::ptr LoggerManager::getLogger(const std::string& name) {
        Spinlock::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end()) {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    }

    std::string LoggerManager::toYamlString() {
        Spinlock::Lock lock(m_mutex);
        YAML::Node node;
        for (auto& i : m_loggers) {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    struct LogAppenderDefine {
        int type = 0; //1 File, 2 Stdout
        LogLevel::Level level = LogLevel::DEBUG;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine& oth) const {
            return type == oth.type
                && level == oth.level
                && formatter == oth.formatter
                && file == oth.file;
        }
    };

    struct LogDefine {
        std::string name;
        LogLevel::Level level = LogLevel::DEBUG;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine& oth) const {
            return name == oth.name
                && level == oth.level
                && formatter == oth.formatter
                && appenders == oth.appenders;
        }

        bool operator<(const LogDefine& oth) const {
            return name < oth.name;
        }
    };

    // 自定义类配置特化
    template<>
    class ConfigCast<std::string, std::set<LogDefine> > {
    public:
        std::set<LogDefine> operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            std::set<LogDefine> vec;
            //node["name"].IsDefined()
            for (size_t i = 0; i < node.size(); ++i) {
                auto n = node[i];
                if (!n["name"].IsDefined()) {
                    std::cout << "log config error: name is null, " << n
                        << std::endl;
                    continue;
                }

                LogDefine ld;
                ld.name = n["name"].as<std::string>();
                ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
                if (n["formatter"].IsDefined()) {
                    ld.formatter = n["formatter"].as<std::string>();
                }

                if (n["appenders"].IsDefined()) {
                    for (size_t x = 0; x < n["appenders"].size(); ++x) {
                        auto a = n["appenders"][x];
                        if (!a["type"].IsDefined()) {
                            std::cout << "log config error: appender type is null, " << a << std::endl;
                            continue;
                        }
                        std::string type = a["type"].as<std::string>();
                        LogAppenderDefine lad;
                        if (type == "FileLogAppender") {
                            lad.type = 1;
                            if (!a["file"].IsDefined()) {
                                std::cout << "log config error: fileappender file is null, " << a << std::endl;
                                continue;
                            }
                            lad.file = a["file"].as<std::string>();
                            if (a["formatter"].IsDefined()) {
                                lad.formatter = a["formatter"].as<std::string>();
                            }
                        }
                        else if (type == "StdoutLogAppender") {
                            lad.type = 2;
                        }
                        else {
                            std::cout << "log config error: appender type is invalid, " << a << std::endl;
                            continue;
                        }
                        ld.appenders.push_back(lad);
                    }
                }

                vec.insert(ld);
            }
            return vec;
        }
    };
    template<>
    class ConfigCast<std::set<LogDefine>, std::string> {
    public:
        std::string operator()(const std::set<LogDefine>& v) {
            YAML::Node node;
            for (auto& i : v) {
                YAML::Node n;
                n["name"] = i.name;
                n["level"] = LogLevel::toString(i.level);
                if (!i.formatter.empty()) {
                    n["level"] = i.formatter;
                }

                for (auto& a : i.appenders) {
                    YAML::Node na;
                    if (a.type == 1) {
                        na["type"] = "FileLogAppender";
                        na["file"] = a.file;
                    }
                    else if (a.type == 2) {
                        na["type"] = "StdoutLogAppender";
                    }
                    na["level"] = LogLevel::toString(a.level);
                    if (!a.formatter.empty()) {
                        na["formatter"] = a.formatter;
                    }
                    n["appenders"].push_back(na);
                }
                node.push_back(n);
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    Framework::ConfigVar<std::set<LogDefine> >::ptr g_log_defines = Framework::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter {
        LogIniter() {
            g_log_defines->addListener([](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value) {
                //LOG_INFO(LOG_ROOT()) << "on logger conf changed";
                for (auto& i : new_value) {
                    auto it = old_value.find(i);
                    Logger::ptr logger;
                    if (it == old_value.end()) {
                        //新增logger
                        //logger.reset(new Logger(i.name));
                        logger = LOG_NAME(i.name);
                    }
                    else {
                        if (!(i == *it)) {
                            //修改的logger
                            logger = LOG_NAME(i.name);
                        }
                    }

                    logger->setLevel(i.level);
                    if (!(i.formatter.empty())) {
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for (auto& a : i.appenders) {
                        LogAppender::ptr ap;
                        if (a.type == 1) {
                            ap.reset(new FileLogAppender(a.file));
                        }
                        else if (a.type == 2) {
                            ap.reset(new StdoutLogAppender);
                        }
                        ap->setLevel(a.level);
                        if (!a.formatter.empty()) {
                            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                            if (!fmt->isError()) {
                                ap->setFormatter(fmt);
                            }
                            else {
                                std::cout << "appender type=" << a.type << " formatter=" << a.formatter << " is invalid" << std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }

                for (auto& i : old_value) {
                    auto it = new_value.find(i);
                    if (it == new_value.end()) {
                        //删除logger
                        auto logger = LOG_NAME(i.name);
                        logger->clearAppenders();
                    }
                }



            });
        }
    };
    static LogIniter log_init;

    void LoggerManager::init() {
    }
}