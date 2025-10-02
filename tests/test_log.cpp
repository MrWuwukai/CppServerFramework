#include <iostream>

#include "log.h"
#include "utils.h"

int main(int argc, char** argv) {
   Framework::Logger::ptr logger(new Framework::Logger);
   logger->addAppender(Framework::LogAppender::ptr(new Framework::StdoutLogAppender));

   // 自定义format模版
   // Framework::logFormatter::ptr fmt(new Framework::LogFormatter("%d");


   /**
   // 创建一个文件日志追加器，将日志写入到"./log.txt"文件中
   Framework::FileLogAppender::ptr file_appender(new Framework::FileLogAppender("./log.txt"));
   // 创建一个日志格式化器，设置日志输出格式为时间 线程ID 日志级别 日志内容 换行
   Framework::LogFormatter::ptr fmt(new Framework::LogFormatter("%d%T%p%T%m%n"));
   // 为文件日志追加器设置格式化器
   file_appender->setFormatter(fmt);
   // 设置文件日志追加器的日志级别为ERROR，即只记录ERROR级别及以上的日志
   file_appender->setLevel(Framework::LogLevel::ERROR);
   // 将文件日志追加器添加到logger中（这里假设logger已正确初始化）
   logger->addAppender(file_appender);
   */
   





   LOG_INFO(logger) << "hello";

   auto l = Framework::loggerMgr::GetInstance()->getLogger("xxx");
   LOG_INFO(l) << "xxx";
   return 0;
}