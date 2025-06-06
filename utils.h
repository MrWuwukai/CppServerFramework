#ifndef __UTILS_H__
#define __UTILS_H__

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <memory>
#include <cstdint>
#include <thread>
#include <iostream>
#include <vector>

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
}

#endif