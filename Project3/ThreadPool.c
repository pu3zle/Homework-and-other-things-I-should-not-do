
#include "ThreadPool.h"
#include <stdio.h>
#include "list.h"
#include <stdlib.h>
#include <string.h>

#include "trace.h"
#include "ThreadPool.tmh"

PFUNC_WORKF
TpWorkerThread(
	_In_ PVOID Context
)
{
	PTHREAD_POOL threadPool = (PTHREAD_POOL)(Context);
	BOOLEAN shouldWork = TRUE;
	DWORD status;
	HANDLE objects[2];
	objects[0] = threadPool->StopEvent;
	objects[1] = threadPool->StartWorkEvent;
	BOOLEAN executing = TRUE;
	printf("[0x%X]Thread awakened\n", GetCurrentThreadId());
	while (executing)
	{
		// wait for the thread pool events
		status = WaitForMultipleObjects(2, objects, FALSE, INFINITE);
		switch (status)
		{
		case STATUS_WAIT_0 + 1:
			// the work event was scheduled
			shouldWork = TRUE;
			break;
		case STATUS_WAIT_0:
		default:
			// the termination event was scheduled
			executing = FALSE;
			continue;
			break;
		}
		while (shouldWork)
		{
			WaitForSingleObject(threadPool->Mutex, INFINITE);
			if (!IsListEmpty(&threadPool->ListHead))
			{
				LIST_ENTRY* entry = RemoveTailList(&threadPool->ListHead);
				ReleaseMutex(threadPool->Mutex);
				PWORK_ITEM workItem = CONTAINING_RECORD(entry,
					WORK_ITEM, ListEntry);
				__try
				{
					workItem->WorkFunction(workItem->Context);
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
				}
				free(workItem);
			}
			else
			{
				ReleaseMutex(threadPool->Mutex);
				shouldWork = FALSE;
			}
		}
	}
	return 0;
}

BOOL
ThreadPoolInit(
	PTHREAD_POOL ThreadPool,
	UINT NoThreads
)
{
	UINT i = 0;
	if (NoThreads > MAX_NO_THREADS)
	{
		printf("Number of threads exceeds maximum supported threads\n");
		return FALSE;
	}

	ThreadPool->StopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (!ThreadPool->StopEvent)
	{
		printf("CreateEvent failed for StopEvent with status: 0x%X\n", GetLastError());
		return FALSE;
	}
	ThreadPool->StartWorkEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	if (!ThreadPool->StartWorkEvent)
	{
		printf("CreateEvent failed for StartWorkEvent with status: 0x%X\n", GetLastError());
		return FALSE;
	}
	ThreadPool->NumberOfThreads = NoThreads;
	ThreadPool->ThreadHandles = (HANDLE*)malloc(NoThreads * sizeof(HANDLE));
	if (!ThreadPool->ThreadHandles)
	{
		printf("Failed to initialize enough memory for the threads");
		return FALSE;
	}

	InitializeListHead(&ThreadPool->ListHead);
	ThreadPool->Mutex = CreateMutexA(NULL, FALSE, NULL);
	if (!ThreadPool->Mutex)
	{
		printf("Failed to initialize mutex\n");
		free(ThreadPool->ThreadHandles);
		return FALSE;
	}

	for (i = 0; i < NoThreads; i++)
	{
		DWORD tid = 0;
		ThreadPool->ThreadHandles[i] = CreateThread(
			NULL,
			0,
			(LPTHREAD_START_ROUTINE)TpWorkerThread,
			(PVOID)ThreadPool,
			0,
			&tid
		);
		if (NULL == ThreadPool->ThreadHandles[i])
		{
			ThreadPool->NumberOfThreads = i;
			CloseHandle(ThreadPool->Mutex);
			ThreadPoolUninit(ThreadPool);
		}
	}
	ThreadPool->IsStarted = TRUE;
	return TRUE;
}

void
ThreadPoolUninit(
	PTHREAD_POOL	ThreadPool
)
{
	DWORD i = 0;
	if (!ThreadPool->StopEvent)
	{
		ThreadPool->IsStarted = FALSE;
		return;
	}
	SetEvent(ThreadPool->StopEvent);

	WaitForMultipleObjects(ThreadPool->NumberOfThreads, ThreadPool->ThreadHandles, TRUE, INFINITE);

	for (i = 0; i < ThreadPool->NumberOfThreads; i++)
	{
		if (ThreadPool->ThreadHandles[i])
		{
			CloseHandle(ThreadPool->ThreadHandles[i]);
		}
	}
	free(ThreadPool->ThreadHandles);
	CloseHandle(ThreadPool->Mutex);

	// Flushing
	while (!IsListEmpty(&ThreadPool->ListHead))
	{
		LIST_ENTRY* entry = RemoveHeadList(&ThreadPool->ListHead);
		PWORK_ITEM workItem = CONTAINING_RECORD(entry, WORK_ITEM, ListEntry);
		workItem->WorkFunction(workItem->Context);
		free(workItem);
	}

	ThreadPool->IsStarted = FALSE;
	return;
}

BOOL
TpEnqueueWorkItem(
	_In_ PTHREAD_POOL ThreadPool,
	_In_ PFUNC_WORKF StartRoutine,
	_Inout_opt_ PVOID Context
)
{
	// allocate a new work item and initialize it
	PWORK_ITEM workItem = (PWORK_ITEM)malloc(sizeof(WORK_ITEM));
	if (!workItem)
	{
		return FALSE;
	}
	workItem->WorkFunction = StartRoutine;
	workItem->Context = Context;

	WaitForSingleObject(ThreadPool->Mutex, INFINITE);
	{
		// Insert the work item in the list	
		InsertHeadList(&ThreadPool->ListHead, &workItem->ListEntry);
	}
	ReleaseMutex(ThreadPool->Mutex);

	// Signal the work scheduled event
	SetEvent(ThreadPool->StartWorkEvent);
	return TRUE;
}

PFUNC_WORKF
TestWorkFunc(
	PVOID Context
)
{
	printf("[0x%X]This does nothing, but it's nice\n", GetCurrentThreadId());
	return 0;
}

void
TpPreInitThreadPool(
	PTHREAD_POOL ThreadPool
)
{
	ThreadPool->IsStarted = FALSE;
	ThreadPool->Mutex = INVALID_HANDLE_VALUE;
	ThreadPool->NumberOfThreads = 0;
	ThreadPool->StartWorkEvent = INVALID_HANDLE_VALUE;
	ThreadPool->StopEvent = INVALID_HANDLE_VALUE;
	ThreadPool->ThreadHandles = NULL;
}