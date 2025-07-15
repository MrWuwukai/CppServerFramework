#include "log.h"
#include "scheduler.h"

static Framework::Logger::ptr g_logger = LOG_ROOT();

void test_fiber()
{
    LOG_INFO(g_logger) << "test in fiber";
    sleep(1);
    Framework::Scheduler::GetThis()->schedule(&test_fiber); // 循环调度这个任务，最终这个任务会在多个协程之间调度执行，这回是协程1做，下一回是协程2做
}

int main(int argc, char** argv) {
    Framework::Scheduler sc(3);  // 3个线程 
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    return 0;
}