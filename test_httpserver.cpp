#include "httpserver.h"
#include "log.h"

static Framework::Logger::ptr g_logger = LOG_ROOT();

void run() {
    Framework::HTTP::HttpServer::ptr server(new Framework::HTTP::HttpServer);
    Framework::Address::ptr addr = Framework::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(2);
    }
    server->start();
}

int main(int argc, char** argv) {
    Framework::IOManager iom(2);
    iom.schedule(run);
    return 0;
}