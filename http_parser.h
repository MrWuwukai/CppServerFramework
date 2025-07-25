#pragma once
#include <memory>
#include <string>

#include "http.h"
#include "httprequest_parser.h"
#include "httprespond_parser.h"

namespace Framework {
    namespace HTTP {
        class HttpRequestParser {
        public:
            typedef std::shared_ptr<HttpRequestParser> ptr;
            HttpRequestParser();
            //size_t execute(const char* data, size_t len, size_t off);
            size_t execute(char* data, size_t len);
            int isFinished();
            int hasError();

            HttpRequest::ptr getData() const { return m_data; }
            const http_parser& getParser() const { return m_parser; }
            uint64_t getContentLength();

            void setError(int v) { m_error = v; }
        public:
            static uint64_t GetHttpRequestBufferSize();
            static uint64_t GetHttpRequestMaxBodySize();
        private:
            http_parser m_parser;
            HttpRequest::ptr m_data;
            int m_error;
        };

        class HttpResponseParser {
        public:
            typedef std::shared_ptr<HttpResponseParser> ptr;
            HttpResponseParser();
            size_t execute(char* data, size_t len, bool chunk);
            int isFinished();
            int hasError();

            HttpResponse::ptr getData() const { return m_data; }
            const httpclient_parser& getParser() const { return m_parser; }
            uint64_t getContentLength();

            void setError(int v) { m_error = v; }
        public:
            static uint64_t GetHttpResponseBufferSize();
            static uint64_t GetHttpResponseMaxBodySize();
        private:
            httpclient_parser m_parser;
            HttpResponse::ptr m_data;
            int m_error;
        };
    }
}