/*----------------------------------------------------------------------------*\
| Threads (mutex)                                                              |
|                                                                              |
| last update: 2011.02.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(MUTEX_H_INCLUDED)
#define MUTEX_H_INCLUDED

struct MutexInternal;

class Mutex
{
public:
	Mutex();
	~Mutex();
	void Lock();
	void Unlock();

	void __init__();
	void __delete__();
private:
	MutexInternal *internal;
};


#endif
