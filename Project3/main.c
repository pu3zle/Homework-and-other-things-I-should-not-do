#define _CRT_SECURE_NO_WARNINGS

#include "ThreadPool.h"
#include "list.h"
#include <stdio.h>
#include <string.h>
#include "Proc_Info.h"


#include "trace.h"
#include "main.tmh"


const char* help_string = "\n\
Commands: \n\
help - brings up the help menu \n\
start - starts the thread pool \n\
stop - stops the thread pool \n\
enqueue - enqueues 100 items in the thread pool \n\
exit - exits the program \n";


int main(int argc, char* args[])
{
	WPP_INIT_TRACING(NULL);
	CHAR command[100];
	THREAD_POOL threadPool;
	HMODULE ntdllLib = INVALID_HANDLE_VALUE;

	TpPreInitThreadPool(&threadPool);
	ntdllLib = LoadNtdllFunctions();
	if (!ntdllLib)
	{
		printf("Failed to load ntdll functions!");
		return 1;
	}

	while (1)
	{
		printf("\n>>");
		scanf("%s", command);

		if (0 == strcmp(command, "help"))
		{
			printf(help_string);
			AppLogInfo("Called \'help\' command");
		}
		else if (0 == strcmp(command, "exit"))
		{
			if (threadPool.IsStarted)
			{
				ThreadPoolUninit(&threadPool);
			}
			printf("\'exit\' command called\n");
			AppLogInfo("Called \'exit\' command");
			goto exit;
		}
		else if (0 == strcmp(command, "start"))
		{
			AppLogInfo("Called \'start\' command");
			if (!threadPool.IsStarted)
			{
				printf("Starting thread pool\n");
				ThreadPoolInit(&threadPool, 5);
			}
			else
			{
				printf("Already started\n");
			}
		}
		else if (0 == strcmp(command, "stop"))
		{
			AppLogInfo("Called \'stop\' command");
			if (threadPool.IsStarted)
			{
				printf("Stopping thread pool\n");
				ThreadPoolUninit(&threadPool);
			}
			else
			{
				printf("Already stopped\n");
			}
		}
		else if (0 == strcmp(command, "enqueue"))
		{
			AppLogInfo("Called \'enqueue\' command");
			if (!threadPool.IsStarted)
			{
				printf("Thread pool not started!\n");
			}
			else
			{
				printf("Enqueueing 10000 tasks...\n");
				int i = 0;
				for (i = 0; i < 10000; i++)
				{
					TpEnqueueWorkItem(&threadPool, (PVOID)TestWorkFunc, NULL);
				}
			}

		}
		else if (0 == strcmp(command, "proclist"))
		{
			AppLogInfo("Called \'proclist\' command");
			GetProcessList();
		}
		else
		{
			AppLogInfo("Called an invalid command");
			printf("Invalid command called\n");
		}
	}

exit:
	FreeNtdllFunctions(ntdllLib);
	WPP_CLEANUP();
}