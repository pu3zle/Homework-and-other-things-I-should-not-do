#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "ThreadPool.h"
#include "internals.h"

typedef struct _IMPORTED_FUNCTIONS {
	_NtQueryInformationProcess fNtQueryInformationProcess;
	_NtQuerySystemInformation fNtQuerySystemInformation;
	_NtUnmapViewOfSection fNtUnmapViewOfSection;

}IMPORTED_FUNCTIONS, *PIMPORTED_FUNCTIONS;

typedef struct _GLOBALS {

	THREAD_POOL ThreadPool;
	HMODULE NtdllModule;
	IMPORTED_FUNCTIONS Functions;

}GLOBALS, *PGLOBALS;

BOOL
InitGlobals();

#endif