#include "httpserver.h"
#include "log.h"

namespace Framework {
    namespace HTTP {
        static Logger::ptr g_logger = LOG_NAME("system");

        HttpServer::HttpServer(bool keepAlive
            , Framework::IOManager* handleClientWorker
            , Framework::IOManager* acceptWorker)
            : TcpServer(handleClientWorker, acceptWorker)
            , m_isKeepAlive(keepAlive) {
            m_dispatch.reset(new ServletDispatch());
        }

        void HttpServer::handleClient(Socket::ptr client) {
            HttpSession::ptr session(new HttpSession(client));
            do {
                auto req = session->recvRequest();
                if (!req) {
                    LOG_WARN(g_logger) << "recv http request fail, errno="
                        << errno << " errstr=" << strerror(errno)
                        << " client:" << client->toString();
                    break;
                }

                HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepAlive));

                m_dispatch->handle(req, rsp, session);
                /*handle里不直接response，这里是切面程序思想。可能在handle的before和after都有其他事情需要处理。*/

                /*test*/
                //rsp->setBody("hello world");

                session->sendResponse(rsp);
            } while (m_isKeepAlive);
            session->close();
        }
    }
}