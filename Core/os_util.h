#ifndef OSUTIL_H
#define OSUTIL_H

#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
      //define something for Windows (64-bit only)
   #else
      //define something for Windows (32-bit only)
   #endif
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
      #define __IOS_IPHONE__ 1
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
      #define __MAC__ 1
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    // android
#elif __linux__
    // linux
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
#endif

#include <stdint.h>
#include "ngx_type.h"

extern ngx_int_t   ngx_ncpu;

void os_init();

int cpu_count();
void thread_affinity_cpu(int cpuid);
void process_affinity_cpu(int cpuid);

#endif
