#include <iostream>
#include "log.h"
#include "utils.h"

int main(int argc, char** ​argv) {
    Framework::Logger::ptr logger(new Framework::Logger);
    logger->addAppender(Framework::LogAppender::ptr(new Framework::StdoutLogAppender));

    LOG_INFO(logger) << "hello";
    return 0;
}