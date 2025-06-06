#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <memory>
#include <cstdint>
#include <thread>

#ifdef _WIN32
#include <functional>
#include <windows.h>
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
}

#endif