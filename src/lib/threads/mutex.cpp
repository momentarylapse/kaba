#include "threads.h"
#include "../file/file.h"

#ifdef OS_WINDOWS
	#include <windows.h>
#endif
#ifdef OS_LINUX
	#include <pthread.h>
#endif


//------------------------------------------------------------------------------
// mutexes


struct sMutex
{
	bool used;
	int index;
#ifdef OS_WINDOWS
	HANDLE mutex;
#endif
#ifdef OS_LINUX
	pthread_mutex_t mutex;
#endif
};

Array<sMutex> Mutex;

sMutex *get_new_mutex()
{
	for (int i=0;i<Mutex.num;i++)
		if (!Mutex[i].used){
			Mutex[i].used = true;
			return &Mutex[i];
		}
	sMutex t;
	t.used = true;
	t.index = Mutex.num;
	Mutex.add(t);
	return &Mutex.back();
}

#ifdef OS_WINDOWS

int MutexCreate()
{
	sMutex *m = get_new_mutex();
	m->mutex = CreateMutex(NULL, false, NULL);
	return m->index;
}

void MutexLock(int mutex)
{
	if (mutex < 0)
		return;
	WaitForSingleObject(Mutex[mutex].mutex, INFINITE);
}

void MutexUnlock(int mutex)
{
	if (mutex < 0)
		return;
	ReleaseMutex(Mutex[mutex].mutex);
}

#endif
#ifdef OS_LINUX

int MutexCreate()
{
	sMutex *m = get_new_mutex();
	pthread_mutex_init(&m->mutex, NULL);
	return m->index;
}

void MutexLock(int mutex)
{
	if (mutex < 0)
		return;
	pthread_mutex_lock(&Mutex[mutex].mutex);
}

void MutexUnlock(int mutex)
{
	if (mutex < 0)
		return;
	pthread_mutex_unlock(&Mutex[mutex].mutex);
}
#endif

