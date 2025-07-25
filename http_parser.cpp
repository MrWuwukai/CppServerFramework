#include "http_parser.h"
#include "log.h"
#include "config.h"

#include <string.h>

namespace {
    static Framework::Logger::ptr g_logger = LOG_NAME("system");

    // 防止请求头和请求体过大，不然就当做是恶意请求
    static Framework::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
        Framework::Config::Lookup("http.request.buffer_size", (uint64_t)(4 * 1024), "http request buffer size");

    static Framework::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
        Framework::Config::Lookup("http.request.max_body_size", (uint64_t)(64 * 1024 * 1024), "http request max body size");

    static Framework::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
        Framework::Config::Lookup("http.response.buffer_size", (uint64_t)(4 * 1024), "http response buffer size");

    static Framework::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
        Framework::Config::Lookup("http.response.max_body_size", (uint64_t)(64 * 1024 * 1024), "http response max body size");

    // 确保这些配置项初始化
    static uint64_t s_http_request_buffer_size = 0;
    static uint64_t s_http_request_max_body_size = 0;
    static uint64_t s_http_response_buffer_size = 0;
    static uint64_t s_http_response_max_body_size = 0;
    struct _RequestSizeIniter {
        _RequestSizeIniter() {
            s_http_request_buffer_size = g_http_request_buffer_size->getValue();
            s_http_request_max_body_size = g_http_request_max_body_size->getValue();
            g_http_request_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv) {
                    s_http_request_buffer_size = nv;
                });

            g_http_request_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv) {
                    s_http_request_max_body_size = nv;
                });
        }
    };
    struct _ResponseSizeIniter {
        _ResponseSizeIniter() {
            s_http_response_buffer_size = g_http_response_buffer_size->getValue();
            s_http_response_max_body_size = g_http_response_max_body_size->getValue();
            g_http_response_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv) {
                    s_http_response_buffer_size = nv;
                });

            g_http_response_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv) {
                    s_http_response_max_body_size = nv;
                });
        }
    };
    static _RequestSizeIniter _init0;
    static _ResponseSizeIniter _init1;
}

namespace Framework {
    namespace HTTP {
        void on_request_method(void* data, const char* at, size_t length) {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
            HttpMethod m = CharsToHttpMethod(at);

            if (m == HttpMethod::INVALID_METHOD) {
                LOG_WARN(g_logger) << "invalid http request method " << std::string(at, length);
                parser->setError(1000);
                return;
            }
            parser->getData()->setMethod(m);
        }

        void on_request_uri(void* data, const char* at, size_t length) {

        }

        void on_request_fragment(void* data, const char* at, size_t length) {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
            parser->getData()->setFragment(std::string(at, length));
        }

        void on_request_path(void* data, const char* at, size_t length) {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
            parser->getData()->setPath(std::string(at, length));
        }

        void on_request_query(void* data, const char* at, size_t length) {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
            parser->getData()->setQuery(std::string(at, length));
        }

        void on_request_version(void* data, const char* at, size_t length) {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
            uint8_t v = 0;
            if (strncmp(at, "HTTP/1.1", length) == 0) {
                v = 0x11;
            }
            else if (strncmp(at, "HTTP/1.0", length) == 0) {
                v = 0x10;
            }
            else {
                LOG_WARN(g_logger) << "invalid http request version: " << std::string(at, length);
                parser->setError(1001);
                return;
            }
            parser->getData()->setVersion(v);
        }

        void on_request_header_done(void* data, const char* at, size_t length) {
            //HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
        }

        void on_request_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen) {
            HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
            if (flen == 0) {
                LOG_WARN(g_logger) << "invalid http request field length == 0";
                //parser->setError(1002);
                return;
            }
            parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
        }

        HttpRequestParser::HttpRequestParser() 
            : m_error(0) {
            m_data.reset(new Framework::HTTP::HttpRequest);
            http_parser_init(&m_parser);
            m_parser.request_method = on_request_method;
            m_parser.request_uri = on_request_uri;
            m_parser.fragment = on_request_fragment;
            m_parser.request_path = on_request_path;
            m_parser.query_string = on_request_query;
            m_parser.http_version = on_request_version;
            m_parser.header_done = on_request_header_done;
            m_parser.http_field = on_request_http_field;
            m_parser.data = this;
        }

        // 1: 成功
        // -1: 有错误
        // >0: 已处理的字节数，且data有效数据为len - v;
        size_t HttpRequestParser::execute(char* data, size_t len) {
            size_t offset = http_parser_execute(&m_parser, data, len, 0);
            // 还没解析完就把已经解析的移走
            memmove(data, data + offset, (len - offset));
            return offset;
        }

        int HttpRequestParser::isFinished() {
            return http_parser_finish(&m_parser);
        }

        int HttpRequestParser::hasError() {
            return m_error || http_parser_has_error(&m_parser);
        }

        uint64_t HttpRequestParser::getContentLength() {
            return m_data->getHeaderAs<uint64_t>("content-length", 0);
        }

        uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
            return s_http_request_buffer_size;
        }

        uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
            return s_http_request_max_body_size;
        }
    }

    namespace HTTP {
        void on_response_reason(void* data, const char* at, size_t length) {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
            parser->getData()->setReason(std::string(at, length));
        }

        void on_response_status(void* data, const char* at, size_t length) {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
            HttpStatus status = (HttpStatus)(atoi(at));
            parser->getData()->setStatus(status);
        }

        void on_response_chunk(void* data, const char* at, size_t length) {
        }

        void on_response_version(void* data, const char* at, size_t length) {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
            uint8_t v = 0;
            if (strncmp(at, "HTTP/1.1", length) == 0) {
                v = 0x11;
            }
            else if (strncmp(at, "HTTP/1.0", length) == 0) {
                v = 0x10;
            }
            else {
                LOG_WARN(g_logger) << "invalid http response version: " << std::string(at, length);
                parser->setError(1001);
                return;
            }

            parser->getData()->setVersion(v);
        }

        void on_response_header_done(void* data, const char* at, size_t length) {
        }

        void on_response_last_chunk(void* data, const char* at, size_t length) {
        }

        void on_response_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen) {
            HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
            if (flen == 0) {
                LOG_WARN(g_logger) << "invalid http response field length == 0";
                //parser->setError(1002);
                return;
            }
            parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
        }

        HttpResponseParser::HttpResponseParser()
            : m_error(0) {
            m_data.reset(new Framework::HTTP::HttpResponse);
            httpclient_parser_init(&m_parser);
            m_parser.reason_phrase = on_response_reason;
            m_parser.status_code = on_response_status;
            m_parser.chunk_size = on_response_chunk;
            m_parser.http_version = on_response_version;
            m_parser.header_done = on_response_header_done;
            m_parser.last_chunk = on_response_last_chunk;
            m_parser.http_field = on_response_http_field;
            m_parser.data = this;
        }

        size_t HttpResponseParser::execute(char* data, size_t len, bool chunk) {
            if (chunk) {
                httpclient_parser_init(&m_parser);
            }
            size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
            memmove(data, data + offset, (len - offset));
            return offset;
        }

        int HttpResponseParser::isFinished() {
            return httpclient_parser_finish(&m_parser);
        }

        int HttpResponseParser::hasError() {
            return m_error || httpclient_parser_has_error(&m_parser);
        }

        uint64_t HttpResponseParser::getContentLength() {
            return m_data->getHeaderAs<uint64_t>("content-length", 0);
        }

        uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
            return s_http_response_buffer_size;
        }

        uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
            return s_http_response_max_body_size;
        }
    }
}