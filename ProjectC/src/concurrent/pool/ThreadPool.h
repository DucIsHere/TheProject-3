#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdalign.h>

#if defined(_WIN32) || defined(_WIN64)
#define PCCORE_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#include <synchapi.h>

#endif
