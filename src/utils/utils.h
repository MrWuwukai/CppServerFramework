#ifndef __UTILS_H__
#define __UTILS_H__

#define _CRT_SECURE_NO_WARNINGS

#include <execinfo.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/fiber/all.hpp> 

namespace Framework {
	uint32_t GetThreadId();
	uint32_t GetFiberId();

	void Backtrace(std::vector<std::string>& bt, int size, int skip); // 获取函数层级，方便查询报错位置
	std::string BacktraceToString(int size, int skip, const std::string& prefix = "");

	uint64_t GetCurrentMS(); // Timer
	uint64_t GetCurrentUS(); // Timer
}

#endif