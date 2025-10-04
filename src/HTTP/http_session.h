#pragma once

#include "http.h"
#include "stream.h"

namespace Framework {
    namespace HTTP {
        class HttpSession : public SocketStream {
        public:
            typedef std::shared_ptr<HttpSession> ptr;
            HttpSession(Socket::ptr sock, bool owner = true);
            HttpRequest::ptr recvRequest();
            int sendResponse(HttpResponse::ptr rsp);
        };
    }
}