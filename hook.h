#pragma once
#include "iomanager.h"
#include <unistd.h>

namespace Framework {
	bool is_hook_enable();
	void set_hook_enable(bool flag);

}

extern "C" {
	/*extern "C"��һ��C++���е��﷨������ָ�������������ĺ������������C���Ե����ӹ���������ӣ�������C++�����ӹ���*/
	typedef unsigned int (*sleep_fun)(unsigned int seconds);
	extern sleep_fun sleep_f;

	typedef int (*usleep_fun)(useconds_t usec);
	extern usleep_fun usleep_f;
}