#pragma once
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "log.h"

/*
C++20 引入了标准的属性 [[likely]] 和 [[unlikely]]，可以替代这种编译器特定的宏定义。
if (condition) [[likely]] {
    // 编译器倾向于认为 condition 为真
} else [[unlikely]] {
    // 编译器倾向于认为 condition 为假
}
*/
// 编译器加速优化的宏
#if defined __GNUC__ || defined __llvm__
#define LIKELY(x)       __builtin_expect(!!(x), 1)
#define UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

// 断言宏
#define ASSERT_W(x, w) if(UNLIKELY(!(x))) { LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\n" << w << "\nbacktrace:\n" << Framework::BacktraceToString(100, 2, "    "); assert(x); }
#define ASSERT(x) if(UNLIKELY(!(x))) { LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n" << Framework::BacktraceToString(100, 2, "    "); assert(x); }