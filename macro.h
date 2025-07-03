#pragma once
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "log.h"

/*
C++20 �����˱�׼������ [[likely]] �� [[unlikely]]������������ֱ������ض��ĺ궨�塣
if (condition) [[likely]] {
    // ��������������Ϊ condition Ϊ��
} else [[unlikely]] {
    // ��������������Ϊ condition Ϊ��
}
*/
// �����������Ż��ĺ�
#if defined __GNUC__ || defined __llvm__
#define LIKELY(x)       __builtin_expect(!!(x), 1)
#define UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

// ���Ժ�
#define ASSERT_W(x, w) if(UNLIKELY(!(x))) { LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\n" << w << "\nbacktrace:\n" << Framework::BacktraceToString(100, 2, "    "); assert(x); }
#define ASSERT(x) if(UNLIKELY(!(x))) { LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n" << Framework::BacktraceToString(100, 2, "    "); assert(x); }