#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "iomanager.h"
#include "log.h"

Framework::Logger::ptr g_logger = LOG_ROOT();

int sock = 0;

void test_fiber() {
    LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "103.235.46.102", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        Framework::IOManager::GetThis()->addEvent(sock, Framework::IOManager::READ, [](){
            LOG_INFO(g_logger) << "read callback";
        });
        Framework::IOManager::GetThis()->addEvent(sock, Framework::IOManager::WRITE, [](){
            LOG_INFO(g_logger) << "write callback";
            //close(sock);
            Framework::IOManager::GetThis()->cancelEvent(sock, Framework::IOManager::READ);
            close(sock);
        });
    } else {
        LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

}

void test() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    Framework::IOManager iom(2, false);
    iom.schedule(&test_fiber);
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
    Framework::IOManager iom(4);
    timer = iom.addTimer(500, []() {
        LOG_INFO(g_logger) << "hello timer";
        static int i = 0;
        if (++i == 5) {
            timer->reset(2000, true);
            // timer->cancel();
        }
        }, true);
}

int main(int argc, char** argv) {
    testTimer();
    // test();
    return 0;
}