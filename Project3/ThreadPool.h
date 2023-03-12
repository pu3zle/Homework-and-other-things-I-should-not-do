#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H

#include <windows.h>

#define MAX_NO_THREADS 20

typedef struct _THREAD_POOL
{
	BOOL IsStarted;
	HANDLE StopEvent;
	HANDLE StartWorkEvent;

	UINT NumberOfThreads;
	HANDLE* ThreadHandles;

	HANDLE Mutex;
	LIST_ENTRY ListHead;

}THREAD_POOL, * PTHREAD_POOL;

typedef DWORD(_stdcall* PFUNC_WORKF)(PVOID Context);

typedef struct _WORK_ITEM
{
	LIST_ENTRY ListEntry;
	PFUNC_WORKF WorkFunction;
	PVOID Context;
}WORK_ITEM, * PWORK_ITEM;

void
ThreadPoolUninit(
	PTHREAD_POOL	ThreadPool
);

BOOL
ThreadPoolInit(
	PTHREAD_POOL ThreadPool,
	UINT NoThreads
);

PFUNC_WORKF
TestWorkFunc(
	PVOID Context
);

BOOL
TpEnqueueWorkItem(
	_In_ PTHREAD_POOL ThreadPool,
	_In_ PFUNC_WORKF StartRoutine,
	_Inout_opt_ PVOID Context
);

void
TpPreInitThreadPool(
	PTHREAD_POOL ThreadPool
);

#endif