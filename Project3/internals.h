#ifndef _INTERNALS_H_
#define _INTERNALS_H_

#include <Windows.h>

#define NTSTATUS DWORD

struct PROCESS_BASIC_INFORMATION {
	PVOID Reserved1;
	DWORD PebBaseAddress;
	PVOID Reserved2[2];
	DWORD UniqueProcessId;
	PVOID Reserved3;
};

typedef NTSTATUS(WINAPI* _NtUnmapViewOfSection)(
	HANDLE ProcessHandle,
	PVOID BaseAddress
	);

typedef NTSTATUS(WINAPI* _NtQueryInformationProcess)(
	HANDLE ProcessHandle,
	DWORD ProcessInformationClass,
	PVOID ProcessInformation,
	DWORD ProcessInformationLength,
	PDWORD ReturnLength
	);

typedef NTSTATUS(WINAPI* _NtQuerySystemInformation)(
	DWORD SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);
#endif // _INTERNALS_H_