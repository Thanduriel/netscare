#pragma once

#include <Windows.h>


#ifdef INJECT_EX_EXPORTS
#define HOOKDLL_API __declspec(dllexport)
#else
#define HOOKDLL_API __declspec(dllimport)
#endif


extern int HOOKDLL_API g_bSubclassed;
HOOKDLL_API int InjectDll(HWND hWnd);
HOOKDLL_API int UnmapDll();
