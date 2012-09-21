/*----------------------------------------------------------------------------*\
| Threads (work scheduler)                                                     |
|                                                                              |
| last update: 2011.02.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(WORK_H_INCLUDED)
#define WORK_H_INCLUDED


#define MAX_THREADS			8

typedef void thread_work_func_t(int);
typedef void scheduled_work_func_t(int, int);

// hight level
bool WorkDo(thread_work_func_t *func, thread_status_func_t *status_func = 0);

//bool WorkSchedule(int work_id, int work_size);

// very high level
bool WorkDoScheduled(scheduled_work_func_t *func, thread_status_func_t *status_func, int _work_size_, int _work_partition_);
int WorkGetNumThreads();
int WorkGetTotal();
int WorkGetDone();


#endif

