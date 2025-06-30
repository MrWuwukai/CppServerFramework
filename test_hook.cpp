#include "hook.h"
#include "log.h"
#include "iomanager.h"

Framework::Logger::ptr g_logger = LOG_ROOT();

void test_sleep() {
    Framework::IOManager iom(1);
    iom.schedule([]() {
        sleep(2);
        LOG_INFO(g_logger) << "sleep 2";
        });

    iom.schedule([]() {
        sleep(3);
        LOG_INFO(g_logger) << "sleep 3";
        });
    LOG_INFO(g_logger) << "test_sleep";
}

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if (rt) {
        return;
    }
}

int main(int argc, char** argv) {
    //test_sleep();
    Framework::IOManager iom;
    iom.schedule(test_sock);
    return 0;
}