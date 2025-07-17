#include "connection.h"
#include "http_parser.h"
#include "utils.h"
#include "log.h"

namespace Framework {
    namespace HTTP {
        static Framework::Logger::ptr g_logger = LOG_NAME("system");

        HttpConnection::~HttpConnection() {
            LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
        }

        HttpResult::ptr HttpConnection::GET(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            Uri::ptr uri = Uri::CreateUri(url);
            if (!uri) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
            }
            return GET(uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::GET(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::POST(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            Uri::ptr uri = Uri::CreateUri(url);
            if (!uri) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
            }
            return POST(uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::POST(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            Uri::ptr uri = Uri::CreateUri(url);
            if (!uri) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
            }
            return DoRequest(method, uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(uri->getPath());
            req->setQuery(uri->getQuery());
            req->setFragment(uri->getFragment());
            req->setMethod(method);
            bool has_host = false;
            for (auto& i : headers) {
                if (strcasecmp(i.first.c_str(), "connection") == 0) {
                    if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                        req->setClose(false);
                    }
                    continue;
                }

                if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
                    has_host = !i.second.empty();
                }

                req->setHeader(i.first, i.second);
            }
            if (!has_host) {
                req->setHeader("Host", uri->getHost());
            }
            req->setBody(body);
            return DoRequest(req, uri, timeout_ms);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms) {
            // URI -> address
            Address::ptr addr = uri->createAddressFromUri();
            if (!addr) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                    , nullptr, "invalid host: " + uri->getHost());
            }
            // create socket
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAILED
                    , nullptr, "connect fail: " + addr->toString());
            }
            // connection
            if (!sock->connect(addr)) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAILED
                    , nullptr, "connect fail: " + addr->toString());
            }
            sock->setRecvTimeout(timeout_ms);
            HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
            // send
            int rt = conn->sendRequest(req);
            if (rt == 0) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE
                    , nullptr, "send request closed by peer: " + addr->toString());
            }
            if (rt < 0) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_ERROR
                    , nullptr, "send request socket error errno=" + std::to_string(errno)
                    + " errstr=" + std::string(strerror(errno)));
            }
            // recv
            auto rsp = conn->recvResponse();
            if (!rsp) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::RECV_TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms));
            }
            return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
        }

        HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
            :SocketStream(sock, owner) {
            m_createTime = Framework::GetCurrentMS();
        }

        HttpResponse::ptr HttpConnection::recvResponse() {
            HttpResponseParser::ptr parser(new HttpResponseParser);
            uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
            std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr) {
                delete[] ptr;
                });
            char* data = buffer.get();
            int offset = 0;

            // head
            do {
                int len = read(data + offset, buff_size - offset);
                if (len <= 0) {
                    close();
                    return nullptr;
                }
                len += offset;
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, false);
                if (parser->hasError()) {
                    close();
                    return nullptr;
                }
                offset = len - nparse;
                if (offset == buff_size) {
                    close();
                    return nullptr;
                }
                if (parser->isFinished()) {
                    break;
                }
            } while (true);
            // chunk，被压缩的数据，不根据getContentLength
            auto& client_parser = parser->getParser();
            if (client_parser.chunked) {
                std::string body;
                int len = offset;
                // all chunk
                do {
                    // ome chunk
                    do {
                        int rt = read(data + len, buff_size - len);
                        if (rt <= 0) {
                            close();
                            return nullptr;
                        }
                        len += rt;
                        data[len] = '\0';
                        size_t nparse = parser->execute(data, len, true);
                        if (parser->hasError()) {
                            close();
                            return nullptr;
                        }
                        len -= nparse;
                        if (len == (int)buff_size) {
                            close();
                            return nullptr;
                        }
                    } while (!parser->isFinished());

                    len -= 2; // 去掉/r/n
                    if (client_parser.content_len <= len) {
                        body.append(data, client_parser.content_len);
                        memmove(data, data + client_parser.content_len, static_cast<size_t>(len) - client_parser.content_len);
                        len -= client_parser.content_len;
                    }
                    else {
                        body.append(data, len);
                        int left = client_parser.content_len - len;
                        while (left > 0) {
                            int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                            if (rt <= 0) {
                                close();
                                return nullptr;
                            }
                            body.append(data, rt);
                            left -= rt;
                        }
                        len = 0;
                    }
                } while (!client_parser.chunks_done);
                parser->getData()->setBody(body);
            }
			else {
				// body，根据getContentLength
				int64_t length = parser->getContentLength();
				if (length > 0) {
					std::string body;
					body.resize(length);

					int len = 0;
					if (length >= offset) {
						memcpy(&body[0], data, offset);
						len = offset;
						//body.append(data, offset);
					}
					else {
						memcpy(&body[0], data, length);
						len = length;
						//body.append(data, length);
					}
					length -= offset;
					if (length > 0) {
						if (readFixSize(&body[len], length) <= 0) {
                            close();
							return nullptr;
						}
					}
					parser->getData()->setBody(body);
				}
			}
            
            return parser->getData();
        }

        int HttpConnection::sendRequest(HttpRequest::ptr rsp) {
            std::stringstream ss;
            ss << rsp->toString();
            std::string data = ss.str();
            return writeFixSize(data.c_str(), data.size());
        }
    }
}

namespace Framework {
    namespace HTTP {
        HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request)
            : m_host(host)
            , m_vhost(vhost)
            , m_port(port)
            , m_maxSize(max_size)
            , m_maxAliveTime(max_alive_time)
            , m_maxRequest(max_request) {
        }

        HttpResult::ptr HttpConnectionPool::GET(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            return DoRequest(HttpMethod::GET, url, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::GET(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            std::stringstream ss;
            ss << uri->getPath()
                << (uri->getQuery().empty() ? "" : "?")
                << uri->getQuery()
                << (uri->getFragment().empty() ? "" : "#")
                << uri->getFragment();
            return GET(ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::POST(const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            return DoRequest(HttpMethod::POST, url, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::POST(Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            std::stringstream ss;
            ss << uri->getPath()
                << (uri->getQuery().empty() ? "" : "?")
                << uri->getQuery()
                << (uri->getFragment().empty() ? "" : "#")
                << uri->getFragment();
            return POST(ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::DoRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(url);
            req->setMethod(method);
            req->setClose(false);
            bool has_host = false;
            for (auto& i : headers) {
                if (strcasecmp(i.first.c_str(), "connection") == 0) {
                    if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                        req->setClose(false);
                    }
                    continue;
                }

                if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
                    has_host = !i.second.empty();
                }

                req->setHeader(i.first, i.second);
            }
            if (!has_host) {
                if (m_vhost.empty()) {
                    req->setHeader("Host", m_host);
                }
                else {
                    req->setHeader("Host", m_vhost);
                }
            }
            req->setBody(body);
            return DoRequest(req, timeout_ms);
        }

        HttpResult::ptr HttpConnectionPool::DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms, const std::map<std::string, std::string>& headers, const std::string& body) {
            std::stringstream ss;
            ss << uri->getPath()
                << (uri->getQuery().empty() ? "" : "?")
                << uri->getQuery()
                << (uri->getFragment().empty() ? "" : "#")
                << uri->getFragment();
            return DoRequest(method, ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::DoRequest(HttpRequest::ptr req, uint64_t timeout_ms) {
            auto conn = getConnection();
            if (!conn) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_CONNECT_FAILED
                    , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
            }
            // get sock from conn
            auto sock = conn->getSocket();
            if (!sock) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION
                    , nullptr, "connect fail: " + sock->getRemoteAddress()->toString());
            }
            sock->setRecvTimeout(timeout_ms);
            HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
            // send
            int rt = conn->sendRequest(req);
            if (rt == 0) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE
                    , nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
            }
            if (rt < 0) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_ERROR
                    , nullptr, "send request socket error errno=" + std::to_string(errno)
                    + " errstr=" + std::string(strerror(errno)));
            }
            // recv
            auto rsp = conn->recvResponse();
            if (!rsp) {
                return std::make_shared<HttpResult>((int)HttpResult::Error::RECV_TIMEOUT
                    , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms));
            }
            return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
        }

        HttpConnection::ptr HttpConnectionPool::getConnection() {
            /*从连接池里取出一个未超时的连接，没有则创建一个*/
            uint64_t now_ms = Framework::GetCurrentMS();
            std::vector<HttpConnection*> invalid_conns;
            HttpConnection* ptr = nullptr;

            Mutex::Lock lock(m_mutex);
            while (!m_conns.empty()) {
                auto conn = *m_conns.begin();
                m_conns.pop_front();
                if (!conn->isConnected()) {
                    invalid_conns.push_back(conn);                  
                    continue;
                }
                if ((conn->m_createTime + m_maxAliveTime) < now_ms) {
                    invalid_conns.push_back(conn);
                    continue;
                }
                ptr = conn;
                break;
            }

            lock.unlock();
            for (auto i : invalid_conns) {
                delete i;
            }
            m_total -= invalid_conns.size();

            if (!ptr) {
                IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
                if (!addr) {
                    LOG_ERROR(g_logger) << "get addr fail: " << m_host;
                    return nullptr;
                }
                addr->setPort(m_port);
                Socket::ptr sock = Socket::CreateTCP(addr);
                if (!sock) {
                    LOG_ERROR(g_logger) << "create sock fail: " << addr->toString();
                    return nullptr;
                }
                if (!sock->connect(addr)) {
                    LOG_ERROR(g_logger) << "sock connect fail: " << addr->toString();
                    return nullptr;
                }

                ptr = new HttpConnection(sock);
                ++m_total;
            }
            // 返回一个智能指针，并绑定他的删除器，指定用ReleasePtr方法删除该智能指针
            return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
        }

        // 不析构，直接返回连接池
        void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
            ++ptr->m_request;
            if (!ptr->isConnected() || ((ptr->m_createTime + pool->m_maxAliveTime) >= Framework::GetCurrentMS()) || (ptr->m_request >= pool->m_maxRequest)) {
                delete ptr;
                --pool->m_total;
                return;
            }
            Mutex::Lock lock(pool->m_mutex);
            pool->m_conns.push_back(ptr);
        }
    }
}