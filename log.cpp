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
    // Logger��Ĺ��캯��������һ���ַ�������name�����ڳ�ʼ����־��¼��������
    Logger::Logger(const std::string& name)
        : m_name(name) {
    }

    // ����־��¼�������һ����־�������appender��
    void Logger::addAppender(LogAppender::ptr appender) {
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
            for (auto& i : m_appenders) {
                i->log(level, event);
            }
        }
    }

    // ���Լ������־��¼����������log������ָ������ΪDEBUG
    void Logger::debug(LogEvent::ptr event) {
        debug(LogLevel::DEBUG, event);
    }

    // ��Ϣ�������־��¼����������log������ָ������ΪINFO
    void Logger::info(LogEvent::ptr event) {
        debug(LogLevel::INFO, event);
    }

    // ���漶�����־��¼����������log������ָ������ΪWARN
    void Logger::warn(LogEvent::ptr event) {
        debug(LogLevel::WARN, event);
    }

    // ���󼶱����־��¼����������log������ָ������ΪERROR
    void Logger::error(LogEvent::ptr event) {
        debug(LogLevel::ERROR, event);
    }

    // �������󼶱����־��¼����������log������ָ������ΪFATAL
    void Logger::fatal(LogEvent::ptr event) {
        debug(LogLevel::FATAL, event);
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
    void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
        // �жϵ�ǰ��־�����Ƿ���ڵ����趨����־����
        if (level >= m_level) {
            // �����������������ʽ�������־�¼�д�뵽�ļ���m_filestream��
            m_filestream << m_formatter->format(level, event);
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
    void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
        // �жϵ�ǰ��־�����Ƿ���ڵ������õ���־����
        if (level >= m_level) {
            // ����ʽ�������־�¼��������׼�����std::cout
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
                if (isspace(m_pattern[n])) {
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
            }

            if (fmt_status == 0) {
                if (!nstr.empty()) {
                    // ���nstr��Ϊ�գ���nstr��һ�����ַ�����ɵ�pair������������0���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                    vec.push_back(std::make_tuple(nstr, "", 0));
                }
                // ��m_pattern�ַ����н�ȡ������i + 1��ʼ������Ϊn - i - 1�����ַ�������ֵ��str
                str = m_pattern.substr(i + 1, n - i - 1);
                // ��str��fmt��ɵ�tuple������������1���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                vec.push_back(std::make_tuple(str, fmt, 1));
                // ������i����Ϊn���������ں���ѭ�����ƻ��ʾ���������
                i = n;
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
                }
                // ��str��fmt��ɵ�tuple������������1���ܱ�ʾĳ��״̬���ʶ����ӵ�vec������
                vec.push_back(std::make_tuple(str, fmt, 1));
                // ������i����Ϊn���������ں���ѭ�����ƻ��ʾ���������
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