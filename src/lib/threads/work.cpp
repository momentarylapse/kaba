#include "../file/file.h"
#include "threads.h"
#include "mutex.h"
#include "work.h"


static int OverwriteThreadNum = -1;

class WorkerThread : public Thread
{
public:
	WorkerThread(int _id)
	{
		id = _id;
	}
	bool Schedule()
	{
		if (!work->mx_list)
			work->mx_list = new Mutex;
		work->mx_list->Lock();
		num = 0;
		if (work->work_given >= work->total_size){
			work->mx_list->Unlock();
			return false;
		}
		first = work->work_given;
		num = min(work->total_size - work->work_given, work->partition_size);
		work->work_given += num;
		work->mx_list->Unlock();
		return true;
	}
	virtual void OnRun()
	{
		while(Schedule()){
			for (int i=0;i<num;i++)
				work->DoStep(first + i);
		}
	}

	ThreadedWork *work;
	int id;
	int first, num;
};

ThreadedWork::ThreadedWork(int _total_size, int _partition_size)
{
	total_size = _total_size;
	partition_size = _partition_size;
	work_given = 0;

	// use max. number of cores?
	int num_threads = ThreadGetNumCores();
	if (OverwriteThreadNum >= 0)
		num_threads = OverwriteThreadNum;

	for (int i=0;i<num_threads;i++)
		thread.add(new WorkerThread(i));
}

ThreadedWork::~ThreadedWork()
{
	foreach(Thread *t, thread)
		delete(t);
}

bool ThreadedWork::Run()
{
	msg_db_r("WorkDo", 1);

	// run threads
	for (int i=0;i<thread.num;i++)
		thread[i]->Run();

	// main program: update gui
	bool all_done = false;
	bool thread_abort = false;
	while((!all_done) && (!thread_abort)){

/*#ifdef _X_USE_HUI_
		HuiDoSingleMainLoop();
		HuiSleep(30);
#endif*/

		thread_abort = !OnStatus();
		all_done = true;
		for (int i=0;i<thread.num;i++)
			all_done &= thread[i]->IsDone();
	}

	if (!thread_abort){
		for (int i=0;i<thread.num;i++)
			thread[i]->Join();
	}

	msg_db_l(1);
	return !thread_abort;
}

int ThreadedWork::GetTotal()
{
	return total_size;
}

int ThreadedWork::GetDone()
{
	return work_given;
}

#if 0

#if 1

void work_func(void *p)
{
	int work_id = (int)(long)p;
	cur_work_func(work_id);
}


bool WorkDo(thread_work_func_t *func, thread_status_func_t *status_func)
{
	msg_db_r("WorkDo", 1);
	// use max. number of cores?
	num_threads = ThreadGetNumCores();
	if (OverwriteThreadNum >= 0)
		num_threads = OverwriteThreadNum;

	// run threads
	cur_work_func = func;
	for (int i=0;i<num_threads;i++){
		work_thread[i] = new Thread;
		work_thread[i]->Call(work_func, (void*)i);
	}
	
	// main program: update gui
	bool all_done = false;
	bool thread_abort = false;
	while((!all_done) && (!thread_abort)){
		
#ifdef _X_USE_HUI_
		HuiDoSingleMainLoop();
		HuiSleep(30);
#endif

		if (status_func)
			thread_abort = !status_func();
		all_done = true;
		for (int i=0;i<num_threads;i++)
			all_done &= work_thread[i]->IsDone();
	}

	if (!thread_abort){
		for (int i=0;i<num_threads;i++)
			work_thread[i]->Join();
	}
	for (int i=0;i<num_threads;i++)
		delete(work_thread[i]);
	
	msg_db_l(1);
	return !thread_abort;
}
#else
bool WorkRunThreads(thread_func *func, thread_status_func *status_func)
{
	// run threads
	num_threads_used = 1;
	for (int i=0;i<num_threads_used;i++){
		thread_data[i].func = func;
		thread_data[i].thread_id = i;
	}
	
	func(0, NULL);

	return true;
}
#endif



scheduled_work_func_t *scheduled_work_func;

bool WorkSchedule(int work_id, int work_size)
{
	if (!mx_work_list)
		mx_work_list = new Mutex;
	mx_work_list->Lock();
	thread_work[work_id].clear();
	if (work_given >= work_size){
		mx_work_list->Unlock();
		return false;
	}
	for (int i=work_given;i<work_size;i++){
		thread_work[work_id].add(i);
		work_given ++;
		if (thread_work[work_id].num >= work_partition)
			break;
	}
	mx_work_list->Unlock();
	return true;
}

void do_thread_scheduled_work(int work_id)
{
	while(WorkSchedule(work_id, work_size)){
		for (int i=0;i<thread_work[work_id].num;i++){
			int w = thread_work[work_id][i];
			scheduled_work_func(work_id, w);
		}
	}
}

bool WorkDoScheduled(scheduled_work_func_t *func, thread_status_func_t *status_func, int _work_size_, int _work_partition_)
{
	work_given = 0;
	work_size = _work_size_;
	work_partition = _work_partition_;
	scheduled_work_func = func;
	for (int i=0;i<MAX_THREADS;i++)
		thread_work[i].clear();

	return WorkDo(do_thread_scheduled_work, status_func);
}

int WorkGetNumThreads()
{
	return num_threads;
}

int WorkGetTotal()
{
	return work_size;
}

int WorkGetDone()
{
	return work_given;
}
#endif
