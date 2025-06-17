#pragma once
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "log.h"

#define ASSERT_W(x, w) if(!(x)) { LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\n" << w << "\nbacktrace:\n" << Framework::BacktraceToString(100, 2, "    "); assert(x); }
#define ASSERT(x) if(!(x)) { LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n" << Framework::BacktraceToString(100, 2, "    "); assert(x); }