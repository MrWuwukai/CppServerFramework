#ifndef __UTILS_H__
#define __UTILS_H__

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <memory>
#include <cstdint>
#include <thread>
#include <iostream>
#include <vector>
#include <string>
#include <execinfo.h>

#ifdef _WIN32
#include <functional>
#include <windows.h>
#include <stdlib.h>
#include <cstdarg>
#else
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <boost/fiber/all.hpp> 
#endif

namespace Framework {
	uint32_t GetThreadId();
	uint32_t GetFiberId();
	#ifdef _WIN32
	int myvasprintf(char** strp, const char* format, va_list ap);
	#endif

	void Backtrace(std::vector<std::string>& bt, int size, int skip); // 获取函数层级，方便查询报错位置
	std::string BacktraceToString(int size, int skip, const std::string& prefix = "");
}

#endif