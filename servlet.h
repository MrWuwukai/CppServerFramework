#pragma once

#include <memory>
#include <functional>
#include <string>

#include "http.h"
#include "session.h"
#include "multithread.h"

namespace Framework {
    namespace HTTP {
        class Servlet {
        public:
            typedef std::shared_ptr<Servlet> ptr;

            Servlet(const std::string& name)
                : m_name(name) {
            }
            virtual ~Servlet() {}
            virtual int32_t handle(Framework::HTTP::HttpRequest::ptr request
                , Framework::HTTP::HttpResponse::ptr response
                , Framework::HTTP::HttpSession::ptr session) = 0;

            const std::string& getName() const { return m_name; }
        protected:
            std::string m_name;
        };

        class FunctionServlet : public Servlet {
        public:
            typedef std::shared_ptr<FunctionServlet> ptr;
            typedef std::function<int32_t(Framework::HTTP::HttpRequest::ptr request
                                        , Framework::HTTP::HttpResponse::ptr response
                                        , Framework::HTTP::HttpSession::ptr session)> callback;

            FunctionServlet(callback cb);
            virtual int32_t handle(Framework::HTTP::HttpRequest::ptr request
                                 , Framework::HTTP::HttpResponse::ptr response
                                 , Framework::HTTP::HttpSession::ptr session) override;
        private:
            callback m_cb;
        };

        // servlet的管理器
        class ServletDispatch : public Servlet {
        public:
            typedef std::shared_ptr<ServletDispatch> ptr;

            ServletDispatch();
            virtual int32_t handle(Framework::HTTP::HttpRequest::ptr request
                , Framework::HTTP::HttpResponse::ptr response
                , Framework::HTTP::HttpSession::ptr session) override;

            void addMatchedServlet(const std::string& uri, Servlet::ptr slt);
            void addMatchedServlet(const std::string& uri, FunctionServlet::callback cb);
            void addFuzzyServlet(const std::string& uri, Servlet::ptr slt);
            void addFuzzyServlet(const std::string& uri, FunctionServlet::callback cb);

            void delMatchedServlet(const std::string& uri);
            void delFuzzyServlet(const std::string& uri);

            Servlet::ptr getDefault() const { return m_default; }
            void setDefault(Servlet::ptr v) { m_default = v; }

            Servlet::ptr getMatchedServlet(const std::string& uri);
            Servlet::ptr getFuzzyServlet(const std::string& uri);
            Servlet::ptr getServlet(const std::string& uri);
        private:
            RWMutex m_mutex;
            // uri(精确匹配) -> servlet
            std::unordered_map<std::string, Servlet::ptr> m_datas;
            // uri(模糊匹配) -> servlet
            std::vector<std::pair<std::string, Servlet::ptr> > m_fuzzy;
            // 默认servlet, 所有路径都没匹配到时使用
            Servlet::ptr m_default;
        };

        class DefaultServlet : public Servlet {
        public:
            typedef std::shared_ptr<DefaultServlet> ptr;
            DefaultServlet();
            virtual int32_t handle(Framework::HTTP::HttpRequest::ptr request
                , Framework::HTTP::HttpResponse::ptr response
                , Framework::HTTP::HttpSession::ptr session) override;
        };

        // 还可以进行扩展，例如上传下载文件的Servlet
    }
}