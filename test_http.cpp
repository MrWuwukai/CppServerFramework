#include "http.h"
#include "log.h"

void test() {
    Framework::HTTP::HttpRequest::ptr req(new Framework::HTTP::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    Framework::HTTP::HttpResponse::ptr rsp(new Framework::HTTP::HttpResponse);
    rsp->setHeader("X-X", "baidu");
    rsp->setBody("hello baidu");

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test();
    return 0;
}