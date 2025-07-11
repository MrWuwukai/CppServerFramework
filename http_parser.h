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
            uint64_t getContentLength();

            void setError(int v) { m_error = v; }
        private:
            http_parser m_parser;
            HttpRequest::ptr m_data;
            int m_error;
        };

        class HttpResponseParser {
        public:
            typedef std::shared_ptr<HttpResponseParser> ptr;
            HttpResponseParser();
            size_t execute(char* data, size_t len);
            int isFinished();
            int hasError();

            HttpResponse::ptr getData() const { return m_data; }
            uint64_t getContentLength();

            void setError(int v) { m_error = v; }
        private:
            httpclient_parser m_parser;
            HttpResponse::ptr m_data;
            int m_error;
        };
    }
}