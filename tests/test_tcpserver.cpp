#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include "tcpserver.h"

Framework::Logger::ptr g_logger = LOG_ROOT();

// sudo netstat -tnalp | grep test_
// telnet 127.0.0.1 8033
void run() {
    auto addr = Framework::Address::LookupAny("0.0.0.0:8033");
    // auto addr2 = Framework::UnixAddress::ptr(new Framework::UnixAddress("/tmp/unix_addr"));

    std::vector<Framework::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    Framework::TcpServer::ptr tcp_server(new Framework::TcpServer);
    std::vector<Framework::Address::ptr> fails;
    while (!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
}
int main(int argc, char** argv) {
    Framework::IOManager iom(2);
    iom.schedule(run);
    return 0;
}