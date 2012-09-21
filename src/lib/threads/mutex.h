/*----------------------------------------------------------------------------*\
| Threads (mutex)                                                              |
|                                                                              |
| last update: 2011.02.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(MUTEX_H_INCLUDED)
#define MUTEX_H_INCLUDED


// mutexes
int MutexCreate();
void MutexLock(int mutex);
void MutexUnlock(int mutex);


#endif
