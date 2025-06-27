#include "hook.h"
#include "log.h"

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

int main(int argc, char** argv) {
    test_sleep();
    return 0;
}