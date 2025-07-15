#include "iomanager.h"
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

Framework::Logger::ptr g_logger = LOG_ROOT();

void test_fiber() {
    LOG_INFO(g_logger) << "test_fiber";
}

void test1() {
    Framework::IOManager iom;
    iom.schedule(&test_fiber);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    iom.addEvent(sock, Framework::IOManager::WRITE, []() {
        LOG_INFO(g_logger) << "connected";
        });
    connect(sock, (const sockaddr*) &addr, sizeof(addr));
}

Framework::Timer::ptr timer;
void testTimer() {
    Framework::IOManager iom(2);
    timer = iom.addTimer(500, []() {
        LOG_INFO(g_logger) << "hello timer";
        static int i = 0;
        if (++i == 5) {
            timer->cancel();
        }
        }, true);
}

int main(int argc, char** argv) {
    testTimer();
    return 0;
}