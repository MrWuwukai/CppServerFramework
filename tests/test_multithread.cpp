#include "config.h"
#include "log.h"
#include "multithread.h"

Framework::Logger::ptr g_logger = LOG_ROOT();

void fun1() {
    LOG_INFO(g_logger) << "name: " << Framework::Multithread::GetName()
        << " this.name: " << Framework::Multithread::GetThis()->getName()
        << " id: " << Framework::GetThreadId()
        << " this.id: " << Framework::Multithread::GetThis()->getId();
}

void fun2() {
}

int main(int argc, char** argv) {
    LOG_INFO(g_logger) << "thread test begin";
    std::vector<Framework::Multithread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        Framework::Multithread::ptr thr(new Framework::Multithread(&fun1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for (int i = 0; i < 5; ++i) {
        thrs[i]->join();
    }
    LOG_INFO(g_logger) << "thread test end";
    return 0;
}