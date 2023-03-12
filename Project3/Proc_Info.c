#include "trace.h"
#include "Proc_Info.tmh"

#include "Proc_Info.h"
#include <TlHelp32.h>
#include <stdio.h>

BOOL SetPrivilege(
    LPCSTR lpszPrivilege  // name of privilege to enable/disable
)
{
    HANDLE hToken = 0;
    TOKEN_PRIVILEGES tp = { 0 };
    LUID luid;

    // Get a token for this process.

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES |
        TOKEN_QUERY, &hToken))
    {
        AppLogError("OpenProcessToken failed");
        return FALSE;
    }

    if (!LookupPrivilegeValueA(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup 
        &luid))        // receives LUID of privilege
    {
        AppLogError("LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
        AppLogError("AdjustTokenPrivileges error: %u\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
        AppLogError("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}

void
PrintCommandLineForProcess(
    HANDLE hProcess
)
{
    // Try to allocate buffer 

    HANDLE hHeap = GetProcessHeap();
    DWORD dwSizeNeeded = 0;
    SIZE_T dwBytesRead = 0;
    DWORD dwSize = sizeof(mPROCESS_BASIC_INFORMATION);

    mPPROCESS_BASIC_INFORMATION pbi = (mPPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap,
        HEAP_ZERO_MEMORY,
        dwSize);
    // Did we successfully allocate memory

    if (!pbi) {
        CloseHandle(hProcess);
        return;
    }

    // Attempt to get basic info on process

    DWORD dwStatus = gNtQueryInformationProcess(hProcess,
        ProcessBasicInformation,
        pbi,
        dwSize,
        &dwSizeNeeded);
    // Did we successfully get basic info on process

    mPEB peb = { 0 };
    PWCHAR cmdLineBuffer = NULL;
    mRTL_USER_PROCESS_PARAMETERS userProcParams = { 0 };

    if (!pbi->PebBaseAddress)
    {
        goto cleanup;
    }
    if (!ReadProcessMemory(hProcess, pbi->PebBaseAddress, &peb, sizeof(peb), &dwBytesRead))
    {
        AppLogError("ReadProcessMemory to read PEB");
        goto cleanup;
    }

    if (!ReadProcessMemory(hProcess, peb.ProcessParameters, &userProcParams, sizeof(userProcParams), &dwBytesRead))
    {
        AppLogError("ReadProcessMemory failed to read Process Parameters");
        goto cleanup;
    }
    // We got Process Parameters, is CommandLine filled in
    if (!userProcParams.CommandLine.Length)
    {
        goto cleanup;
    }

    cmdLineBuffer = (WCHAR*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, userProcParams.CommandLine.Length + sizeof(WCHAR));
    if (!cmdLineBuffer)
    {
        AppLogError("Insuffiecient memory. Allocation not possible");
        goto cleanup;
    }

    if (!ReadProcessMemory(hProcess, userProcParams.CommandLine.Buffer, cmdLineBuffer, userProcParams.CommandLine.Length, &dwBytesRead))
    {
        AppLogError("ReadProcessMemory failed to read command line");
        goto cleanup;
    }
    cmdLineBuffer[userProcParams.CommandLine.Length / sizeof(WCHAR)] = L'\0';
    printf("\n  CommandLine = %ls", cmdLineBuffer);
    
cleanup:
    if (cmdLineBuffer)
    {
        HeapFree(hHeap, 0, cmdLineBuffer);
    }
    if (pbi)
    {
        HeapFree(hHeap, 0, pbi);
    }
    if (hProcess)
    {
        CloseHandle(hProcess);
    }
    return;
        
}

BOOL
GetProcessList()
{
	HANDLE hProcessSnap;
    HANDLE hProcess;
    PROCESSENTRY32 pe32;
    DWORD dwPriorityClass;

    BOOL enabledDebug = SetPrivilege("SeDebugPrivilege");
    if (!enabledDebug)
    {
        AppLogError("Failed to enable debug privileges");
    }

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        AppLogError("CreateToolhelp32Snapshot failed");
        return(FALSE);
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        AppLogError("Process32First failed");
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return(FALSE);
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    do
    {
        printf("\n\n=====================================================");
        printf("\nPROCESS NAME:  %ls", pe32.szExeFile);
        printf("\n-------------------------------------------------------");
        printf("\n  Process ID        = %d", pe32.th32ProcessID);
        printf("\n  Thread count      = %d", pe32.cntThreads);
        printf("\n  Parent process ID = %d", pe32.th32ParentProcessID);

        if (enabledDebug)
        {
            if (pe32.th32ProcessID != 4 && pe32.th32ProcessID != 0)
            {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (NULL == hProcess)
                {
                    AppLogError("OpenProcess failed to open process with pid: %d", pe32.th32ProcessID);
                }
                else
                {
                    PrintCommandLineForProcess(hProcess);
                }
            }
        }

    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return(TRUE);
}

HMODULE
LoadNtdllFunctions()
{
    {
        // Load NTDLL Library and get entry address

        // for NtQueryInformationProcess

        HMODULE hNtDll = LoadLibraryA("ntdll.dll");
        if (hNtDll == NULL)
        {
            AppLogError("Failed to load ntdll.dll");
            return NULL;
        }

        gNtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll,
            "NtQueryInformationProcess");
        if (gNtQueryInformationProcess == NULL) {
            AppLogError("GetProcAddress failed to retrieve NtQueryInformationProcess");
            FreeLibrary(hNtDll);
            return NULL;
        }
        return hNtDll;
    }
}

void FreeNtdllFunctions(HMODULE hNtDll)
{
    if (hNtDll)
        FreeLibrary(hNtDll);
    gNtQueryInformationProcess = NULL;
}
