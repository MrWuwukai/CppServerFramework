#pragma once
#include "iomanager.h"
#include <unistd.h>

namespace Framework {
	bool is_hook_enable();
	void set_hook_enable(bool flag);

}

extern "C" {
	/*extern "C"是一个C++特有的语法，用于指定括号内声明的函数或变量按照C语言的链接规则进行链接，而不是C++的链接规则。*/
	typedef unsigned int (*sleep_fun)(unsigned int seconds);
	extern sleep_fun sleep_f;

	typedef int (*usleep_fun)(useconds_t usec);
	extern usleep_fun usleep_f;
}