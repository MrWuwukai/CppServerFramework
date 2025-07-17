#pragma once

#include "http.h"
#include "stream.h"
#include "uri.h"
#include "multithread.h"

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
                POOL_CONNECT_FAILED = 7,
                POOL_INVALID_CONNECTION = 8,
            };

            HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error)
                : result(_result)
                , response(_response)
                , error(_error) {
            }
            int result;
            HttpResponse::ptr response;
            std::string error;
            std::string toString() const {
                std::stringstream ss;
                ss << "[HttpResult result=" << result
                    << " error=" << error
                    << " response=" << (response ? response->toString() : "nullptr")
                    << "]";
                return ss.str();
            }
        };

        class HttpConnectionPool;
        class HttpConnection : public SocketStream {
            friend class HttpConnectionPool; // ��Ҫ����m_createTime
        public:
            typedef std::shared_ptr<HttpConnection> ptr;

            static HttpResult::ptr GET(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr GET(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr POST(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr POST(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms); // ��ǰ������ת������һ��������

            HttpConnection(Socket::ptr sock, bool owner = true);
            ~HttpConnection();
            HttpResponse::ptr recvResponse();
            int sendRequest(HttpRequest::ptr rsp);
        private:
            uint64_t m_createTime = 0;
            uint64_t m_request = 0;
        };

        // ���ݿ����ӳأ�URI�̶�host���������ж������
        // TODO: ��ѯ��Ȩ����ѯ������ʧ���������
        class HttpConnectionPool {
        public:
            typedef std::shared_ptr<HttpConnectionPool> ptr;

            HttpConnectionPool(const std::string& host
                , const std::string& vhost
                , uint32_t port
                , uint32_t max_size
                , uint32_t max_alive_time
                , uint32_t max_request);

            HttpConnection::ptr getConnection();

            HttpResult::ptr GET(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            HttpResult::ptr GET(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            HttpResult::ptr POST(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            HttpResult::ptr POST(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            HttpResult::ptr DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers = {}, const std::string& body = "");
            HttpResult::ptr DoRequest(HttpRequest::ptr req, uint64_t timeout_ms);
        private:
            /*���ӳ��ڴ�ŵ�����ָ�룬����ָ���Ž�����ָ�룬����ָ������ʱ�����Ե������ǵ��������������������ͷţ����ǿ��Դ�Ż����ӳ�*/
            static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);
        private:
            std::string m_host;
            std::string m_vhost;
            uint32_t m_port;
            uint32_t m_maxSize;
            uint32_t m_maxAliveTime;
            uint32_t m_maxRequest;

            Mutex m_mutex;
            std::list<HttpConnection*> m_conns;
            std::atomic<int32_t> m_total = { 0 };         
        };
    }
}