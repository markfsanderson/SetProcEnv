// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{

    OutputDebugString(L"We're here!\n");

	switch (ul_reason_for_call)
	{
	    case DLL_PROCESS_ATTACH:
            OutputDebugString(L"DLL_PROCESS_ATTACH\n");
            SetEnvironmentVariable(L"YOURCOMPUTER", L"P0wned!");
            break;
	    case DLL_THREAD_ATTACH:
            OutputDebugString(L"DLL_THREAD_ATTACH\n");
            break;
	    case DLL_THREAD_DETACH:
            OutputDebugString(L"DLL_THREAD_DETACH\n");
            break;
	    case DLL_PROCESS_DETACH:
            OutputDebugString(L"DLL_PROCESS_DETACH\n");
		    break;
	}
	return TRUE;
}

