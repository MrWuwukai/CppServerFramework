#pragma once

#include "http.h"
#include "stream.h"
#include "uri.h"

namespace Framework {
    namespace HTTP {
        struct HttpResult {
            typedef std::shared_ptr<HttpResult> ptr;
            enum class Error {
                OK = 0,
                INVALID_URL = 1,
                INVALID_HOST = 2,
                CONNECT_FAILED = 3,
                SEND_CLOSE = 4,
                SEND_ERROR = 5,
                RECV_TIMEOUT = 6,
            };

            HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error)
                : result(_result)
                , response(_response)
                , error(_error) {
            }
            int result;
            HttpResponse::ptr response;
            std::string error;
        };

        class HttpConnection : public SocketStream {
        public:
            typedef std::shared_ptr<HttpConnection> ptr;

            static HttpResult::ptr GET(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr GET(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr POST(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr POST(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms); // 将前端请求转发到另一个服务器

            HttpConnection(Socket::ptr sock, bool owner = true);
            HttpResponse::ptr recvResponse();
            int sendRequest(HttpRequest::ptr rsp);
        };
    }
}