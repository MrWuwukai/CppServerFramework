#include <iostream>
#include "log.h"

int main(int argc, char** ​argv) {
    Framework::Logger::ptr logger(new Framework::Logger);
    logger->addAppender(Framework::LogAppender::ptr(new Framework::StdoutLogAppender));

    Framework::LogEvent::ptr event(new Framework::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0)));
    event->getSS() << "hello";
    logger->log(Framework::LogLevel::INFO, event);
    return 0;
}