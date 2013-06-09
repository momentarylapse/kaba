/*----------------------------------------------------------------------------*\
| Threads                                                                      |
|                                                                              |
| last update: 2011.02.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(THREADS_H_INCLUDED)
#define THREADS_H_INCLUDED

// auxiliary
int ThreadGetNumCores();

//typedef void thread_func_t(void*);
//typedef bool thread_status_func_t();

struct ThreadInternal;

class Thread
{
public:
	Thread();
	virtual ~Thread();
	void Run();
	bool IsDone();
	void Kill();
	void Join();

	virtual void OnRun(){}// = 0;

	void __init__();
	void __delete__();

	bool running;
	ThreadInternal *internal;
};

void ThreadExit();
Thread *ThreadSelf();


#endif

