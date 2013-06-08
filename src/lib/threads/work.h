/*----------------------------------------------------------------------------*\
| Threads (work scheduler)                                                     |
|                                                                              |
| last update: 2011.02.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(WORK_H_INCLUDED)
#define WORK_H_INCLUDED


#define MAX_THREADS			32

class ThreadedWork
{
public:
	ThreadedWork(int total_size, int partition_size);
	virtual ~ThreadedWork();
	virtual void DoStep(int index) = 0;
	virtual bool OnStatus(){	return false;	}
	bool Run();

	int total_size, partition_size;
	Array<Thread*> thread;

	int work_given;
	Mutex *mx_list;

	int GetTotal();
	int GetDone();
};


#endif

