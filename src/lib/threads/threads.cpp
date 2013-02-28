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
	bool running;
	void *p;
	thread_func_t *f;
#ifdef OS_WINDOWS
	HANDLE thread;
#endif
#ifdef OS_LINUX
	pthread_t thread;
#endif
};


static Array<Thread*> _Thread_List_;


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

Thread::Thread()
{
	__init__();
}

Thread::~Thread()
{
	__delete__();
}

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


void Thread::__init__()
{
	internal = new ThreadInternal;
	internal->running = false;
	_Thread_List_.add(this);
}

static void *thread_start_func(void *p)
{
	Thread *t = (Thread*)p;
	t->internal->f(t->internal->p);
	t->internal->running = false;
	return NULL;
}

// create and run a new thread
void Thread::Call(thread_func_t *f, void *param)
{
	internal->f = f;
	internal->p = param;
	internal->running = true;
	int ret = pthread_create(&internal->thread, NULL, &thread_start_func, (void*)this);

	if (ret != 0)
		internal->running = false;
}


void Thread::__delete__()
{
	Kill();
	for (int i=0;i<_Thread_List_.num;i++)
		if (_Thread_List_[i] == this)
			_Thread_List_.erase(i);
	delete(internal);
}

void Thread::Kill()
{
	if (internal->running)
		pthread_cancel(internal->thread);
	internal->running = false;
}

void Thread::Join()
{
	if (internal->running)
		pthread_join(internal->thread, NULL);
	internal->running = false;
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


bool Thread::IsDone()
{
	return !internal->running;
}


ThreadBase::ThreadBase(){}

ThreadBase::~ThreadBase(){}

static void _thread_base_run_(void *p)
{
	ThreadBase *t = (ThreadBase*)p;
	t->OnRun();
}

void ThreadBase::Run()
{
	Call(&_thread_base_run_, this);
}

