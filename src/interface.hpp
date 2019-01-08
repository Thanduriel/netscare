#pragma once

#include <Windows.h>


#define INJECT_EX_EXPORTS
#ifdef INJECT_EX_EXPORTS
#define HOOKDLL_API __declspec(dllexport)
#else
#define HOOKDLL_API __declspec(dllimport)
#endif

HOOKDLL_API int InjectDll(HWND hWnd, HANDLE hIn, HANDLE hOut);
HOOKDLL_API int UnmapDll();
