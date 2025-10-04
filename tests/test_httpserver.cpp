#include "http_server.h"
#include "log.h"

static Framework::Logger::ptr g_logger = LOG_ROOT();

//void run() {
//    Framework::HTTP::HttpServer::ptr server(new Framework::HTTP::HttpServer);
//    Framework::Address::ptr addr = Framework::Address::LookupAnyIPAddress("0.0.0.0:8020");
//    while (!server->bind(addr)) {
//        sleep(2);
//    }
//    server->start();
//}

void run() {
    Framework::HTTP::HttpServer::ptr server(new Framework::HTTP::HttpServer);
    Framework::Address::ptr addr = Framework::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(2);
    }

    auto sd = server->getServletDispatch();
    sd->addMatchedServlet("/a", [](Framework::HTTP::HttpRequest::ptr req
        , Framework::HTTP::HttpResponse::ptr rsp
        , Framework::HTTP::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
        });
    sd->addFuzzyServlet("/a/*", [](Framework::HTTP::HttpRequest::ptr req
        , Framework::HTTP::HttpResponse::ptr rsp
        , Framework::HTTP::HttpSession::ptr session) {
            rsp->setBody("F" + req->toString());
            return 0;
        });
    server->start();
}

int main(int argc, char** argv) {
    Framework::IOManager iom(2);
    iom.schedule(run);
    return 0;
}