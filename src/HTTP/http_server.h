#pragma once

#include "http_session.h"
#include "servlet.h"
#include "tcpserver.h"

namespace Framework {
	namespace HTTP {
        class HttpServer : public TcpServer {
        public:
            typedef std::shared_ptr<HttpServer> ptr;
            HttpServer(bool keepAlive = false
                , Framework::IOManager* handleClientWorker = Framework::IOManager::GetThis()
                , Framework::IOManager* acceptWorker = Framework::IOManager::GetThis());

            ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
            void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }
        protected:
            virtual void handleClient(Socket::ptr client) override;
        private:
            bool m_isKeepAlive;
            ServletDispatch::ptr m_dispatch;
        };
	}
}