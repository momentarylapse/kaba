/*----------------------------------------------------------------------------*\
| Threads                                                                      |
|                                                                              |
| last update: 2011.02.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(THREADS_H_INCLUDED)
#define THREADS_H_INCLUDED

// auxiliary
int ThreadGetNumCores();

typedef void thread_func_t(void*);
typedef bool thread_status_func_t();

struct ThreadInternal;

class Thread
{
public:
	Thread();
	~Thread();
	void Call(thread_func_t *f, void *param = 0);
	bool IsDone();
	void Kill();
	void Join();

	void __init__();
	void __delete__();

	ThreadInternal *internal;
};

class ThreadBase : public Thread
{
public:
	ThreadBase();
	virtual ~ThreadBase();
	void Run();
	virtual void OnRun() = 0;
};

void ThreadExit();
Thread *ThreadSelf();


#endif

