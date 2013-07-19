#include "threads.h"
#include "../file/file.h"

#ifdef OS_WINDOWS
	#include <windows.h>
#endif
#ifdef OS_LINUX
	#include <pthread.h>
	#include <unistd.h>
#endif

struct ThreadInternal
{
#ifdef OS_WINDOWS
	HANDLE thread;
#endif
#ifdef OS_LINUX
	pthread_t thread;
#endif
};


static Array<Thread*> _Thread_List_;

#if 0

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

void Thread::__init__()
{
	new(this) Thread;
}

#ifdef OS_WINDOWS


DWORD WINAPI thread_start_func(__in LPVOID p)
{
	Thread *t = (Thread*)p;
	t->OnRun();
	t->running = false;
	return NULL;
}

// create and run a new thread
void Thread::Run()
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

void Thread::Kill()
{
	if (thread < 0)
		return;
	if (Thread[thread].running)
		TerminateThread(Thread[thread].thread, 0);
	Thread[thread].running = false;
}

void Thread::Join()
{
	WaitForSingleObject(internal->thread, INFINITE);
}

void Thread::Exit()
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


void Thread::Thread()
{
	internal = NULL;
	running = false;
	_Thread_List_.add(this);
}

static void *thread_start_func(void *p)
{
	Thread *t = (Thread*)p;
	t->OnRun();
	t->running = false;
	return NULL;
}


// create and run a new thread
void Thread::Run()
{
	if (!internal)
		internal = new ThreadInternal;
	running = true;
	int ret = pthread_create(&internal->thread, NULL, &thread_start_func, (void*)this);

	if (ret != 0)
		running = false;
}


void Thread::__delete__()
{
	Kill();
	for (int i=0;i<_Thread_List_.num;i++)
		if (_Thread_List_[i] == this)
			_Thread_List_.erase(i);
	if (internal)
		delete(internal);
}

Thread::~Thread()
{
	Kill();
	for (int i=0;i<_Thread_List_.num;i++)
		if (_Thread_List_[i] == this)
			_Thread_List_.erase(i);
	if (internal)
		delete(internal);
}

void Thread::Kill()
{
	if (running)
		pthread_cancel(internal->thread);
	running = false;
}

void Thread::Join()
{
	if (running)
		pthread_join(internal->thread, NULL);
	running = false;
}

void ThreadExit()
{
	pthread_exit(NULL);
}

Thread *ThreadSelf()
{
	pthread_t s = pthread_self();
	for (int i=0;i<_Thread_List_.num;i++)
		if (_Thread_List_[i]->internal->thread == s)
			return _Thread_List_[i];
	return NULL;
}


#endif

#endif

bool Thread::IsDone()
{
	return !running;
}








Thread::Thread()
{
}

void Thread::Run()
{
}


void Thread::__init__()
{
}

void Thread::__delete__()
{
}

Thread::~Thread()
{
}

void Thread::Kill()
{
}

void Thread::Join()
{
}

void ThreadExit()
{
}

Thread *ThreadSelf()
{
	return NULL;
}

int ThreadGetNumCores()
{	return 1;	}
