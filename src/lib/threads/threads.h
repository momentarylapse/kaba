/*----------------------------------------------------------------------------*\
| Threads                                                                      |
|                                                                              |
| last update: 2011.02.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(THREADS_H_INCLUDED)
#define THREADS_H_INCLUDED

// auxiliary
int ThreadGetNumCores();

typedef void thread_func_t(int, void*);
typedef bool thread_status_func_t();

// low level
int ThreadCreate(thread_func_t *f, void *param = 0);
//void ThreadRun(int thread);
void ThreadDelete(int thread);
void ThreadKill(int thread);
void ThreadWaitTillDone(int thread);
bool ThreadDone(int thread);
void ThreadExit();
int ThreadGetId();

#include "mutex.h"
#include "work.h"


#endif

