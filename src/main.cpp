#include "renderer.hpp"
#include "hook.hpp"
#include "interface.hpp"

#include <windows.h>
#include <stdio.h>
#include <d3d.h>
#include <d3d11.h>
#include <iostream>
#include <fstream>

bool firstTime = true;

using namespace std;

bool __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain)
{
//	DXGI_FRAME_STATISTICS stats;
//	pSwapChain->GetFrameStatistics(&stats);
	
	cout << "Hooking Present" << endl;
	cout << "" << endl;

	cout << "New m_SwapChain:		0x" << hex << pSwapChain << endl;
	//CHECKING IF SWAPCHAIN IS VALID
	if (!pSwapChain) return false;

	//HOOKING PRESENT
	PresentVTableHook* hook = new PresentVTableHook(pSwapChain);
	hook->AddHook(Device::Present, 8);

	return true;
}

static std::ofstream logFile("log.txt", std::ofstream::out);

bool FindSwapChain()
{
	if (!Device::Initialize())
		return false;

	const DWORD_PTR swapChainVTable = reinterpret_cast<DWORD_PTR>(PresentVTableHook::GetVtable(&Device::GetSwapChain()));
	DWORD_PTR dwVTableAddress = 0;

#define _PTR_MAX_VALUE 0xFFE00000
	MEMORY_BASIC_INFORMATION32 mbi = { 0 };

	for (DWORD_PTR memptr = 0x10000; memptr < _PTR_MAX_VALUE; memptr = mbi.BaseAddress + mbi.RegionSize) //For x64 -> 0x10000 ->  0x7FFFFFFEFFFF
	{
		if (!VirtualQuery(reinterpret_cast<LPCVOID>(memptr), reinterpret_cast<PMEMORY_BASIC_INFORMATION>(&mbi), sizeof(MEMORY_BASIC_INFORMATION))) //Iterate memory by using VirtualQuery
			continue;

		if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || mbi.Protect & PAGE_GUARD) //Filter Regions
			continue;

		const DWORD_PTR len = mbi.BaseAddress + mbi.RegionSize;     //Do once

		for (DWORD_PTR current = mbi.BaseAddress; current < len; ++current)
		{
			// interpret data as pointer
			__try
			{
				dwVTableAddress = *(DWORD_PTR*)current;
			}
			__except (1)
			{
				continue;
			}

			if (dwVTableAddress == swapChainVTable)
			{
				if (current == (DWORD_PTR)&Device::GetSwapChain())
					continue;
				else
				{
				//	cout << current << endl;
					logFile << current << endl;
				//	return hookD3D11Present((IDXGISwapChain*)current);    //If found we hook Present in swapChain
				}
			}
		}
	}
	return false;
}

// **************************************************************** //


#pragma data_seg (".shared")
int		g_bSubclassed = 0;	// START button subclassed?
UINT	WM_HOOKEX = 0;
HWND	g_hWnd = 0;		// handle of START button
HHOOK	g_hHook = 0;
#pragma data_seg ()

#pragma comment(linker,"/SECTION:.shared,RWS")


//-------------------------------------------------------------
// global variables (unshared!)
//
HINSTANCE	hDll;

// New & old window procedure of the subclassed START button
WNDPROC				OldProc = NULL;
LRESULT CALLBACK	NewProc(HWND, UINT, WPARAM, LPARAM);


//-------------------------------------------------------------
// DllMain
//
BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		std::cout << "attached dll" << std::endl;
		hDll = (HINSTANCE)hModule;
		::DisableThreadLibraryCalls(hDll);

		if (WM_HOOKEX == NULL)
			WM_HOOKEX = ::RegisterWindowMessage("WM_HOOKEX_RK");
	}

	return TRUE;
}


#define pCW ((CWPSTRUCT*)lParam)

LRESULT HookProc(
	int code,       // hook code
	WPARAM wParam,  // virtual-key code
	LPARAM lParam   // keystroke-message information
)
{
	if ((pCW->message == WM_HOOKEX) && pCW->lParam)
	{
		::UnhookWindowsHookEx(g_hHook);

		if (g_bSubclassed)
			goto END;		// already subclassed?

		// Let's increase the reference count of the DLL (via LoadLibrary),
		// so it's NOT unmapped once the hook is removed;
		char lib_name[MAX_PATH];
		::GetModuleFileName(hDll, lib_name, MAX_PATH);

		if (!::LoadLibrary(lib_name))
			goto END;

		FindSwapChain();

		// Subclass START button
	/*	OldProc = (WNDPROC)
			::SetWindowLong(g_hWnd, GWL_WNDPROC, (long)NewProc);
		if (OldProc == NULL)			// failed?
			::FreeLibrary(hDll);
		else {						// success -> leave
			::MessageBeep(MB_OK);	// mapped into "explorer.exe"
			g_bSubclassed = true;
		}*/
	}
	else if (pCW->message == WM_HOOKEX)
	{
		::UnhookWindowsHookEx(g_hHook);

		// Failed to restore old window procedure? => Don't unmap the
		// DLL either. Why? Because then "explorer.exe" would call our
		// "unmapped" NewProc and  crash!!
		if (!SetWindowLong(g_hWnd, GWL_WNDPROC, (long)OldProc))
			goto END;

		::FreeLibrary(hDll);

		::MessageBeep(MB_OK);
		g_bSubclassed = false;
	}

END:
	return ::CallNextHookEx(g_hHook, code, wParam, lParam);
}
/*
int __cdecl main(void)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		"demo.exe",        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return 0;
	}

	std::cout << FindSwapChain();

	char c;
	std::cin >> c;

	return 0;
}*/

int InjectDll(HWND hWnd)
{
	g_hWnd = hWnd;

	// Hook
	g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)HookProc,
		hDll, GetWindowThreadProcessId(hWnd, NULL));
	if (g_hHook == NULL)
		return 0;

	// By the time SendMessage returns, 
	// the START button has already been subclassed
	SendMessage(hWnd, WM_HOOKEX, 0, 1);
	
	return g_bSubclassed;
}

int UnmapDll()
{
	g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)HookProc,
		hDll, GetWindowThreadProcessId(g_hWnd, NULL));

	if (g_hHook == NULL)
		return 0;

	SendMessage(g_hWnd, WM_HOOKEX, 0, 0);

	return (g_bSubclassed == NULL);
}

//-------------------------------------------------------------
// NewProc
// Notice:	- new window procedure for the START button;
//			- it just swaps the left & right muse clicks;
//	
LRESULT CALLBACK NewProc(
	HWND hwnd,      // handle to window
	UINT uMsg,      // message identifier
	WPARAM wParam,  // first message parameter
	LPARAM lParam   // second message parameter
)
{
	return CallWindowProc(OldProc, hwnd, uMsg, wParam, lParam);
}