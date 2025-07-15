#pragma once

#include "http.h"
#include "stream.h"

namespace Framework {
    namespace HTTP {
        class HttpConnection : public SocketStream {
        public:
            typedef std::shared_ptr<HttpConnection> ptr;
            HttpConnection(Socket::ptr sock, bool owner = true);
            HttpResponse::ptr recvResponse();
            int sendRequest(HttpRequest::ptr rsp);
        };
    }
}