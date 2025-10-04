#include <fnmatch.h>

#include "servlet.h"

namespace Framework {
    namespace HTTP {
        FunctionServlet::FunctionServlet(callback cb)
            :Servlet("FunctionServlet")
            , m_cb(cb) {
        }

        int32_t FunctionServlet::handle(Framework::HTTP::HttpRequest::ptr request
            , Framework::HTTP::HttpResponse::ptr response
            , Framework::HTTP::HttpSession::ptr session) {
            return m_cb(request, response, session);
        }

        ServletDispatch::ServletDispatch()
            :Servlet("ServletDispatch") {
            m_default.reset(new DefaultServlet());
        }

        int32_t ServletDispatch::handle(Framework::HTTP::HttpRequest::ptr request
            , Framework::HTTP::HttpResponse::ptr response
            , Framework::HTTP::HttpSession::ptr session) {
            auto slt = getServlet(request->getPath());
            if (slt) {
                slt->handle(request, response, session);
            }
            return 0;
        }

        void ServletDispatch::addMatchedServlet(const std::string& uri, Servlet::ptr slt) {
            RWMutex::WriteLock lock(m_mutex);
            m_datas[uri] = slt;
        }

        void ServletDispatch::addMatchedServlet(const std::string& uri, FunctionServlet::callback cb) {
            RWMutex::WriteLock lock(m_mutex);
            m_datas[uri].reset(new FunctionServlet(cb));
        }

        void ServletDispatch::addFuzzyServlet(const std::string& uri
            , Servlet::ptr slt) {
            RWMutex::WriteLock lock(m_mutex);
            for (auto it = m_fuzzy.begin(); it != m_fuzzy.end(); ++it) {
                if (it->first == uri) {
                    m_fuzzy.erase(it);
                    break;
                }
            }
            m_fuzzy.push_back(std::make_pair(uri, slt));
        }

        void ServletDispatch::addFuzzyServlet(const std::string& uri
            , FunctionServlet::callback cb) {
            return addFuzzyServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
        }

        void ServletDispatch::delMatchedServlet(const std::string& uri) {
            RWMutex::WriteLock lock(m_mutex);
            m_datas.erase(uri);
        }

        void ServletDispatch::delFuzzyServlet(const std::string& uri) {
            RWMutex::WriteLock lock(m_mutex);
            for (auto it = m_fuzzy.begin(); it != m_fuzzy.end(); ++it) {
                if (it->first == uri) {
                    m_fuzzy.erase(it);
                    break;
                }
            }
        }

        Servlet::ptr ServletDispatch::getMatchedServlet(const std::string& uri) {
            RWMutex::ReadLock lock(m_mutex);
            auto it = m_datas.find(uri);
            return it == m_datas.end() ? nullptr : it->second;
        }

        Servlet::ptr ServletDispatch::getFuzzyServlet(const std::string& uri) {
            RWMutex::ReadLock lock(m_mutex);
            for (auto it = m_fuzzy.begin(); it != m_fuzzy.end(); ++it) {
                if (it->first == uri) {
                    return it->second;
                }
            }
            return nullptr;
        }

        Servlet::ptr ServletDispatch::getServlet(const std::string& uri) {
            RWMutex::ReadLock lock(m_mutex);
            auto mit = m_datas.find(uri);
            if (mit != m_datas.end()) {
                return mit->second;
            }
            for (auto it = m_fuzzy.begin(); it != m_fuzzy.end(); ++it) {
                if (!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
                    return it->second;
                }
            }
            return m_default;
        }

        DefaultServlet::DefaultServlet()
            : Servlet("NotFound") {
        }

        int32_t DefaultServlet::handle(Framework::HTTP::HttpRequest::ptr request
            , Framework::HTTP::HttpResponse::ptr response
            , Framework::HTTP::HttpSession::ptr session) {

			static const std::string& RSP_BODY = "<html>"
                "<head><title>404 Not Found</title></head>"
                "<body>"
                "<center><h1>404 Not Found</h1></center>"
                "<hr><center>MyServer/1.0.0</center>"
                "</body>"
                "</html>";

            response->setStatus(Framework::HTTP::HttpStatus::NOT_FOUND);
            response->setHeader("Server", "MyServer/1.0.0");
            response->setHeader("Content-Type", "text/html");
            response->setBody(RSP_BODY);
            return 0;
        }
    }
}