#include "Core.h"
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <time.h>
#define __USE_GNU
#include <sched.h>
#include <pthread.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif
#endif

ngx_int_t   ngx_ncpu = 0;

int cpu_count()
{
#ifdef _WIN32
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	// LOGD("Number of processors: %d.\n", info.dwNumberOfProcessors);
	int cpu_num = info.dwNumberOfProcessors;
	return cpu_num;
#elif __MAC__
	return 0;
#else
	//系统总核数
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
	// LOGD("CORE PROCESSORS_CONF=%d\n",cpu_num);
	cpu_num = get_nprocs_conf();
	//系统可使用核数
	cpu_num = get_nprocs();
	cpu_num = sysconf(_SC_NPROCESSORS_ONLN);
	return cpu_num;
#endif
}

void os_init(){
    if (ngx_ncpu == 0) {
        ngx_ncpu = cpu_count();
    }
	if(ngx_ncpu <= 0) ngx_ncpu = 1;
}


//线程绑定cpu
void thread_affinity_cpu(int cpuid)
{
#ifdef _WIN32
	DWORD_PTR ret = SetThreadAffinityMask(GetCurrentThread(),cpuid);
	if(ret == 0)
	{
		LOGE("SetThreadAffinityMask GetLastError:%d",GetLastError());
	}
#elif __MAC__
#else
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpuid, &mask);

	if (pthread_setaffinity_np(pthread_self(), sizeof(mask),&mask) < 0) {
		LOGE("pthread_setaffinity_np error(%d)",errno);
	}
#endif
}

//进程绑定cpu
void process_affinity_cpu(int cpuid)
{
#ifdef _WIN32
#elif __MAC__
#else
	cpu_set_t mask;
	CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) < 0) {
        LOGE("sched_setaffinity error(%d)",errno);
    }
#endif
}
