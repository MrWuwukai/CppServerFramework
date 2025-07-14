#pragma once

#include "tcpserver.h"
#include "session.h"

namespace Framework {
	namespace HTTP {
        class HttpServer : public TcpServer {
        public:
            typedef std::shared_ptr<HttpServer> ptr;
            HttpServer(bool keepAlive = false
                , Framework::IOManager* handleClientWorker = Framework::IOManager::GetThis()
                , Framework::IOManager* acceptWorker = Framework::IOManager::GetThis());
        protected:
            virtual void handleClient(Socket::ptr client) override;
        private:
            bool m_isKeepAlive;
        };
	}
}