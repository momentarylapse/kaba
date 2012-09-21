#include "threads.h"
#include "../file/file.h"

#ifdef OS_WINDOWS
	#include <windows.h>
#endif
#ifdef OS_LINUX
	#include <pthread.h>
	#include <unistd.h>
#endif

struct sThread
{
	bool used, running;
	int index;
	void *p;
	thread_func_t *f;
#ifdef OS_WINDOWS
	HANDLE thread;
#endif
#ifdef OS_LINUX
	pthread_t thread;
#endif
};


Array<sThread> Thread;

sThread *get_new_thread()
{
	for (int i=0;i<Thread.num;i++)
		if (!Thread[i].used){
			Thread[i].used = true;
			return &Thread[i];
		}
	sThread t;
	t.used = true;
	t.index = Thread.num;
	Thread.add(t);
	return &Thread.back();
}



//------------------------------------------------------------------------------
// auxiliary

int ThreadGetNumCores()
{
#ifdef OS_WINDOWS
	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);
	return siSysInfo.dwNumberOfProcessors;
#endif
#ifdef OS_LINUX
	return ::sysconf(_SC_NPROCESSORS_ONLN);
#endif
	return 1;
}




//------------------------------------------------------------------------------
// low level

#ifdef OS_WINDOWS


DWORD WINAPI thread_start_func(__in LPVOID p)
{
	int thread_id = (int)(long)p;
	Thread[thread_id].f(thread_id, Thread[thread_id].p);
	Thread[thread_id].running = false;
	return 0;
}

// create and run a new thread
int ThreadCreate(thread_func_t *f, void *param)
{
	sThread *t = get_new_thread();
	t->f = f;
	t->p = param;
	t->running = true;
	t->thread = CreateThread(NULL, 0, &thread_start_func, (void*)t->index, 0, NULL);

	if (!t->thread){
		t->running = false;
		t->used = false;
		return -1;
	}
	return t->index;
}


void ThreadDelete(int thread)
{
	if (thread < 0)
		return;
	ThreadKill(thread);
	Thread[thread].used = false;
}

void ThreadKill(int thread)
{
	if (thread < 0)
		return;
	if (Thread[thread].running)
		TerminateThread(Thread[thread].thread, 0);
	Thread[thread].running = false;
}

void ThreadWaitTillDone(int thread)
{
	if (thread < 0)
		return;
	WaitForSingleObject(Thread[thread].thread, INFINITE);
}

void ThreadExit()
{
	ExitThread(0);
}

int ThreadGetId()
{
	HANDLE h = GetCurrentThread();
	for (int i=0;i<Thread.size();i++)
		if (h == Thread[i].thread)
			return i;
	return -1;
}


#endif
#ifdef OS_LINUX

void *thread_start_func(void *p)
{
	int thread_id = (int)(long)p;
	Thread[thread_id].f(thread_id, Thread[thread_id].p);
	Thread[thread_id].running = false;
	return NULL;
}

// create and run a new thread
int ThreadCreate(thread_func_t *f, void *param)
{
	sThread *t = get_new_thread();
	t->f = f;
	t->p = param;
	t->running = true;
	int ret = pthread_create(&t->thread, NULL, &thread_start_func, (void*)t->index);

	if (ret != 0){
		t->running = false;
		t->used = false;
		return -1;
	}
	return t->index;
}


void ThreadDelete(int thread)
{
	if (thread < 0)
		return;
	ThreadKill(thread);
	Thread[thread].used = false;
}

void ThreadKill(int thread)
{
	if (thread < 0)
		return;
	if (Thread[thread].running)
		pthread_cancel(Thread[thread].thread);
	Thread[thread].running = false;
}

void ThreadWaitTillDone(int thread)
{
	if (thread < 0)
		return;
	if (Thread[thread].running)
		pthread_join(Thread[thread].thread, NULL);
}

void ThreadExit()
{
	pthread_exit(NULL);
}

int ThreadGetId()
{
	pthread_t s = pthread_self();
	for (int i=0;i<Thread.num;i++)
		if (s == Thread[i].thread)
			return i;
	return -1;
}


#endif


bool ThreadDone(int thread)
{
	if (thread < 0)
		return true;
	return !Thread[thread].running;
}

