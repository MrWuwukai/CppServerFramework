#pragma once

#include <functional>
#include <memory>
#include <string>

#include "http.h"
#include "http_session.h"
#include "multithread.h"

/*
Servlet 的主要作用：
​1. ​处理 HTTP 请求与响应​​
    当用户通过浏览器访问一个网站（比如提交表单、点击链接），这个请求会被发送到 Web 服务器。
    如果该请求是由 Servlet 处理的，Web 服务器就会把请求交给对应的 Servlet 去处理。
    Servlet 接收到请求后，可以进行逻辑处理（比如查询数据库、计算数据等），然后生成响应内容（如 HTML 页面），再返回给客户端。
​2. ​生成动态内容​​
    与静态 HTML 不同，Servlet 可以根据不同的请求参数、用户身份、时间等因素，​​动态生成不同的网页内容​​。
    比如：用户登录后显示其个人信息；根据搜索关键字返回不同的商品列表。
​3. ​作为 Web 应用的控制器​​
    在经典的 Java Web MVC 架构中，Servlet 常被用作 ​​Controller（控制器）​​，负责接收请求、调用业务逻辑、选择视图（比如 JSP 页面）返回给用户。
*/
/*
servlet和CGI的区别是什么？

这是一个非常好的问题！​​Servlet​​ 和 ​​CGI（Common Gateway Interface，通用网关接口）​​ 都是用于在 Web 服务器上处理客户端请求（通常是来自浏览器的 HTTP 请求）
并生成动态内容的技术，但它们的设计理念、实现方式、性能和适用场景上有很大的不同。

CGI 的工作方式：
用户通过浏览器发送一个请求（比如提交表单）。
Web 服务器（如 Apache）​​根据配置，把该请求交给一个外部程序（即 CGI 脚本）去处理​​。
Web 服务器为每个请求 ​​启动一个新的进程​​，调用 CGI 程序（可能是 Perl、C、Python 写的脚本）。
CGI 程序处理请求，生成响应，然后返回给 Web 服务器，最终返回给用户。
请求结束后，该 CGI 进程通常会被销毁。
​​每次请求都启动一个新进程，开销非常大！​​

Servlet 的工作方式：
用户发送 HTTP 请求到 Web 服务器（如 Tomcat）。
Web 容器根据 URL 映射，将请求交给一个 ​​已经加载并常驻内存中的 Servlet 类​​ 去处理。
Servlet 由 Web 容器管理，采用 ​​多线程模型​​，一个 Servlet 实例可以并发处理多个请求（每个请求在一个独立线程中运行）。
Servlet 处理请求，生成动态内容，然后返回响应。
​​不需要为每个请求都创建新进程或新线程（容器管理线程池），效率高。​
*/

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