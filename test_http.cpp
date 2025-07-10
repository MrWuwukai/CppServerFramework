#include "http.h"
#include "log.h"

void test() {
    Framework::HTTP::HttpRequest::ptr req(new Framework::HTTP::HttpRequest);
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello sylar");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    Framework::HTTP::HttpResponse::ptr rsp(new Framework::HTTP::HttpResponse);
    rsp->setHeader("X-X", "sylar");
    rsp->setBody("hello sylar");

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test();
    return 0;
}