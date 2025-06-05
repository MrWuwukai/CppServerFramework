#include "log.h"

namespace Framework {
    // ---LogEvent---
    LogEvent::LogEvent(const char* file, int32_t m_line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time) 
        : m_file(file)
        , m_line(m_line)
        , m_elapse(elapse)
        , m_threadId(thread_id)
        , m_fiberId(fiber_id)
        , m_time(time) {

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

    // ---Logger---
    // Logger��Ĺ��캯��������һ���ַ�������name�����ڳ�ʼ����־��¼��������
    Logger::Logger(const std::string& name)
        : m_name(name), m_level(LogLevel::INFO) {
        m_formatter.reset(new LogFormatter("%d [%p] <%f:%l> %m %n"));
    }

    // ����־��¼�������һ����־�������appender��
    void Logger::addAppender(LogAppender::ptr appender) {
        if (!appender->getFormatter()){
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    // ����־��¼����ɾ��ָ������־�������appender��
    void Logger::delAppender(LogAppender::ptr appender) {
        for (auto it = m_appenders.begin();
            it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    // ��¼��־�ķ�����������־�����ж��Ƿ��¼��������������������������������־���
    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            auto self = shared_from_this();
            for (auto& i : m_appenders) {
                i->log(self, level, event);
            }
        }
    }

    // ���Լ������־��¼����������log������ָ������ΪDEBUG
    void Logger::debug(LogEvent::ptr event) {
        log(LogLevel::DEBUG, event);
    }

    // ��Ϣ�������־��¼����������log������ָ������ΪINFO
    void Logger::info(LogEvent::ptr event) {
        log(LogLevel::INFO, event);
    }

    // ���漶�����־��¼����������log������ָ������ΪWARN
    void Logger::warn(LogEvent::ptr event) {
        log(LogLevel::WARN, event);
    }

    // ���󼶱����־��¼����������log������ָ������ΪERROR
    void Logger::error(LogEvent::ptr event) {
        log(LogLevel::ERROR, event);
    }

    // �������󼶱����־��¼����������log������ָ������ΪFATAL
    void Logger::fatal(LogEvent::ptr event) {
        log(LogLevel::FATAL, event);
    }

    // ---LogAppender---
    // FileLogAppender��Ĺ��캯��������һ���ļ�������
    // ���ڳ�ʼ����Ա����m_filename���ñ����洢��־Ҫд����ļ���
    FileLogAppender::FileLogAppender(const std::string& filename)
        : m_filename(filename) {
    }

    // FileLogAppender���log���������ڽ���־�¼�д���ļ�
    // level��ʾ��־�ļ���
    // event��ָ����־�¼�������ָ��
    void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        // �жϵ�ǰ��־�����Ƿ���ڵ����趨����־����
        if (level >= m_level) {
            // �����������������ʽ�������־�¼�д�뵽�ļ���m_filestream��
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

    // FileLogAppender���reopen�������������´���־�ļ���
    // ����ֵ������ļ����ɹ��򿪣����ļ���������Ч��������true�����򷵻�false
    bool FileLogAppender::reopen() {
        // �ж��ļ�������m_filestream�Ƿ���ڣ���Ϊnullptr��
        if (m_filestream) {
            // �رյ�ǰ���ļ���
            m_filestream.close();
        }
        // ���Դ�ָ���ļ������ļ���
        m_filestream.open(m_filename);
        // �����ļ����Ƿ�ɹ��򿪵�״̬��!m_filestreamΪfalse��ʾ��ʧ�ܣ�true��ʾ�򿪳ɹ�
        return!m_filestream;
    }

    // StdoutLogAppender���log���������ڽ���־�¼��������׼�����
    // level����־����
    // event��ָ����־�¼����������ָ��
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        // �жϵ�ǰ��־�����Ƿ���ڵ������õ���־����
        if (level >= m_level) {
            // ����ʽ�������־�¼��������׼�����std::cout
            std::cout << m_formatter->format(logger, level, event);
        }
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
            os << logger->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadIdFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
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
            // �����ǰ�ַ����� '%', ֱ�ӽ���׷�ӵ� nstr ��
            if (m_pattern[i] != '%') {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            // �����ǰ�ַ��� '%' ����һ���ַ�Ҳ�� '%', ˵����ת��� '%', 
            // ֱ�ӽ� '%' ׷�ӵ� nstr ��Ȼ��������һ���ַ��ļ��
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
                // ��������ո�ֹͣ������ǰ��ʽ�������
                if (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}') {
                    break;
                }
                // ����δ��ʼ������ʽ��ʶʱ
                if (fmt_status == 0) {
                    // ������� '('����ʾ��ʽ��ʶ��ʼ
                    if (m_pattern[n] == '(') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; // ��ǽ����ʽ����״̬
                        fmt_begin = n; // ��¼��ʽ��ʶ��ʼ������
                        ++n;
                        continue;
                    }
                }
                // ���Ѿ���ʼ������ʽ��ʶʱ
                if (fmt_status == 1) {
                    // ������� ')'����ʾ��ʽ��ʶ����
                    if (m_pattern[n] == ')') {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 2; // ��Ǹ�ʽ�������
                        break;
                    }
                }
                ++n;
            }

            if (fmt_status == 0) {
                if (!nstr.empty()) {
                    // ���nstr��Ϊ�գ���nstr��һ�����ַ�����ɵ�pair������������0���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                // ��m_pattern�ַ����н�ȡ������i + 1��ʼ������Ϊn - i - 1�����ַ�������ֵ��str
                str = m_pattern.substr(i + 1, n - i - 1);
                // ��str��fmt��ɵ�tuple������������1���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                vec.push_back(std::make_tuple(str, fmt, 1));
                // ������i����Ϊn���������ں���ѭ�����ƻ��ʾ���������
                i = n - 1;
            }
            else if (fmt_status == 1) {
                // ���fmt_status����1�������ʽ����������Ϣ������ģʽ�ַ���m_pattern�Լ���ǰλ��i�������ַ���
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                // ��һ������"<pattern_error>"��fmt��tuple������������0���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                vec.push_back(std::make_tuple("<pattern_error>", fmt, 0));
            }
            else if (fmt_status == 2) {
                if (!nstr.empty()) {
                    // ���nstr��Ϊ�գ���nstr��һ�����ַ�����ɵ�pair������������0���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                    vec.push_back(std::make_tuple(nstr, "", 0));
                    nstr.clear();
                }
                // ��str��fmt��ɵ�tuple������������1���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                vec.push_back(std::make_tuple(str, fmt, 1));
                // ������i����Ϊn���������ں���ѭ�����ƻ��ʾ���������
                i = n - 1;
            }
        }

        if (!nstr.empty()) {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
            // # �� C/C++ Ԥ���������ַ���������������������ǽ������ת��Ϊ�ַ�����������
            // #str �Ὣ����� str �������� "m"��"p" �ȣ�ת�����ַ������������� "m"��"p"����
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
                XX(l, LineFormatItem)
            #undef XX
        };

        for (auto& i : vec) {
            // �ж�Ԫ��i�ĵ�����Ԫ���Ƿ�Ϊ0
            if (std::get<2>(i) == 0) {
                // �����0������һ��StringFormatItem���󣬲�����ָ���װ��FormatItem::ptr�У�
                // Ȼ����ӵ�m_items�����С��������StringFormatItem��һ���Զ����࣬
                // ���Ĺ��캯������Ԫ��i�ĵ�һ��Ԫ����Ϊ����
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else {
                // ���Ԫ��i�ĵ�����Ԫ�ز�Ϊ0����s_format_items���������������һ��map֮��Ĺ����������в��Ҽ�ΪԪ��i�ĵ�һ��Ԫ��
                auto it = s_format_items.find(std::get<0>(i));
                // ���û���ҵ���Ӧ�ļ�
                if (it == s_format_items.end()) {
                    // ����һ����ʽ��������Ϣ��StringFormatItem���󣬲�����ָ���װ��FormatItem::ptr�У�
                    // ��ӵ�m_items�����С���ʽ����Ϣ����ԭʼ���ҵļ�
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<error_format " + std::get<0>(i) + ">")));
                }
                else {
                    // ����ҵ��˶�Ӧ�ļ��������ҵ���ֵ�ĵڶ���Ԫ�أ�������һ���ɵ��ö��󣬱��纯��ָ���������
                    // ������Ԫ��i�ĵڶ���Ԫ����Ϊ������������ֵ��װ��FormatItem::ptr�У���ӵ�m_items������
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            // ���Ԫ��i�ĵ�һ�����ڶ����͵�����Ԫ�أ����ض��ĸ�ʽ�ָ�������
            std::cout << std::get<0>(i) << " - " << std::get<1>(i) << " - " << std::get<2>(i) << std::endl;
        }
    }

    
}