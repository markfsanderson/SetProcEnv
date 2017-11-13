// SetProcEnv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <winternl.h>
#include <winnt.h>
#include <ntassert.h>
#include <TlHelp32.h>

void DisplayError(PTCHAR msg);

BOOL SetPrivilege(HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
);

DWORD FindProcess(PWCHAR target);
void ResumeProcess(DWORD processId);
void SuspendProcess(DWORD processId);
BOOL SetDebugPrivilege(HANDLE hProcess);

int main()
{
    DWORD dwProcessId = 0;
    DWORD threadId = (DWORD)-1;
    HANDLE hProcess = (HANDLE)-1;
    SIZE_T nBytesWritten = 0;

    HANDLE currentProcess = GetCurrentProcess();

    BOOL success = SetDebugPrivilege(currentProcess);
    if (FALSE == success)
    {
        DisplayError(L"SetDebugPrivilege error!");
        return EXIT_FAILURE;
    }

    dwProcessId = FindProcess(L"notepad.exe");
    if (0 > dwProcessId)
    {
        DisplayError(L"FindProcess error!");
        return EXIT_FAILURE;
    }

    // open the process
    hProcess = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        dwProcessId); // PID from commandline
    if (NULL == hProcess)
    {
        DisplayError(L"OpenProcess");
        return EXIT_FAILURE;
    }

    WCHAR newdll[] = L"IP0wnYou.dll";
    PTHREAD_START_ROUTINE pThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    SIZE_T szPayload = wcslen(newdll) * sizeof(WCHAR) + sizeof(WCHAR);
    PVOID pRemoteMem = VirtualAllocEx(hProcess, NULL, szPayload, MEM_COMMIT, PAGE_READWRITE);

    WriteProcessMemory(hProcess, pRemoteMem, newdll, szPayload, &nBytesWritten);

    SuspendProcess(dwProcessId);
    
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pThreadRtn, pRemoteMem, CREATE_SUSPENDED, &threadId);
    if (NULL == hThread)
    {
        DisplayError(L"CreateRemoteThread error!");
        return EXIT_FAILURE;
    }
    CloseHandle(hProcess);

    HANDLE hMutex = CreateMutexEx(NULL, L"Local\\ImARealHackerNow", CREATE_MUTEX_INITIAL_OWNER, EVENT_ALL_ACCESS);
    if (NULL == hMutex)
    {
        DisplayError(L"CreateRemoteThread error!");
        return EXIT_FAILURE;
    }

    DWORD err = ResumeThread(hThread);
    UNREFERENCED_PARAMETER(err);
    DWORD retval = WaitForSingleObject(hThread, 10000);   
    if (WAIT_OBJECT_0 == retval)
    {
        DWORD threadRetVal;
        GetExitCodeThread(hThread, &threadRetVal);
        printf("wfso returned 0x%x\n", threadRetVal);
    }

    ResumeProcess(dwProcessId);
    return 0;
}

BOOL SetPrivilege(HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(
        NULL,				// lookup privilege on local system
        lpszPrivilege,		// privilege to lookup 
        &luid))				// receives LUID of privilege
    {        
        DisplayError(L"LookupPrivilegeValue error!");
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
        DisplayError(L"AdjustTokenPrivileges error!");
        return FALSE;
    }

    return TRUE;
}

BOOL SetDebugPrivilege(HANDLE hProcess)
{    
    HANDLE hToken = (HANDLE)-1;
    BOOL ret = FALSE;

    ret = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken);
    if (FALSE == ret)
    {
        DisplayError(L"OpenProcessToken error!");
    }
    ret = SetPrivilege(hToken, SE_DEBUG_NAME, 1);
    if (FALSE == ret)
    {
        DisplayError(L"SetPrivilege error!");
    }     
    return ret;
}

void DisplayError(PTCHAR msg)
{
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, eNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        sysMsg, 256, NULL);

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do { *p-- = 0; } while ((p >= sysMsg) &&
        ((*p == '.') || (*p < 33)));

    // Display the message
    _tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}


DWORD FindProcess(PWCHAR target)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    BOOLEAN foundit = FALSE;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        DisplayError(TEXT("CreateToolhelp32Snapshot (of processes)"));
        return(FALSE);
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        DisplayError(TEXT("Process32First")); // show cause of failure
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return(FALSE);
    }

    // Now walk the snapshot of processes
    do
    {
        if (0 == lstrcmpiW(pe32.szExeFile, target))
        {
            foundit = TRUE;
            break;
        }    
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return foundit ? pe32.th32ProcessID : -1;
}

#pragma region Process State Manipulaton
void SuspendProcess(DWORD processId)
{
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    Thread32First(hThreadSnapshot, &threadEntry);

    do
    {
        if (threadEntry.th32OwnerProcessID == processId)
        {
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE,
                threadEntry.th32ThreadID);

            SuspendThread(hThread);
            CloseHandle(hThread);
        }
    } while (Thread32Next(hThreadSnapshot, &threadEntry));

    CloseHandle(hThreadSnapshot);
}

void ResumeProcess(DWORD processId)
{
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    Thread32First(hThreadSnapshot, &threadEntry);

    do
    {
        if (threadEntry.th32OwnerProcessID == processId)
        {
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE,
                threadEntry.th32ThreadID);

            ResumeThread(hThread);
            CloseHandle(hThread);
        }
    } while (Thread32Next(hThreadSnapshot, &threadEntry));

    CloseHandle(hThreadSnapshot);
}
#pragma endregion