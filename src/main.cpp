#include "renderer.hpp"
#include "hook.hpp"
#include "interface.hpp"

#include <windows.h>
#include <stdio.h>
#include <d3d.h>
#include <d3d11.h>
#include <iostream>
#include <fstream>
#include <set>

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
	auto oldPresent = hook->AddHook(Device::Present, 8);
	Device::SetOrgPresent(oldPresent);

	return true;
}

// __try does not like objects with destructors
static std::ofstream logFile("log.txt", std::ofstream::out);
static std::set<DWORD_PTR> foundObjects;

static void ResetFoundObjects() { foundObjects.clear(); }
static bool Insert(DWORD_PTR _ptr) { return foundObjects.insert(_ptr).second; }
static size_t CountObjects() { return foundObjects.size(); }

bool FindSwapChain()
{
	if (!Device::Initialize())
		return false;

	ResetFoundObjects();

	const DWORD_PTR swapChainVTable = reinterpret_cast<DWORD_PTR>(PresentVTableHook::GetVtable(&Device::GetSwapChain()));
	DWORD_PTR dwVTableAddress = 0;
	int objectCount = 0;

#define _PTR_MAX_VALUE 0xFFE00000
	MEMORY_BASIC_INFORMATION32 mbi = { 0 };
	DWORD fisrAddr = 0x0;
	int count = 0;


	for (DWORD_PTR memptr = 0x10000; memptr < _PTR_MAX_VALUE; memptr = mbi.BaseAddress + mbi.RegionSize) //For x64 -> 0x10000 ->  0x7FFFFFFEFFFF
	{
		++count;
		if (!VirtualQuery(reinterpret_cast<LPCVOID>(memptr), reinterpret_cast<PMEMORY_BASIC_INFORMATION>(&mbi), sizeof(MEMORY_BASIC_INFORMATION))) {//Iterate memory by using VirtualQuery 
			MessageBox(NULL, "Fail Virtual Query", "Fail", MB_OK);
			break;
		}
		if (fisrAddr ==  mbi.BaseAddress) {
			char c[30];
			sprintf_s<30>(c, "%i count", count);
			MessageBox(NULL, c , "found start", MB_OK);
			break;
		}
		if (!fisrAddr) fisrAddr = mbi.BaseAddress;
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
				else if (Insert(current))
				{
					logFile << current << endl;
					if(objectCount == 1) hookD3D11Present((IDXGISwapChain*)current);
				}
				else if(objectCount > 10)// found object for second time
				{
					// return true;
				}
				++objectCount;
			}
		}
	}
	return false;
}

// **************************************************************** //


#pragma data_seg (".shared")
int		g_isHooked = 0;	// START button subclassed?
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

		if (g_isHooked)
			goto END;		// already subclassed?

		// Let's increase the reference count of the DLL (via LoadLibrary),
		// so it's NOT unmapped once the hook is removed;
		char lib_name[MAX_PATH];
		::GetModuleFileName(hDll, lib_name, MAX_PATH);

		if (!::LoadLibrary(lib_name))
			goto END;

		FindSwapChain();
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
		g_isHooked = false;
	}

END:
	return ::CallNextHookEx(g_hHook, code, wParam, lParam);
}

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
	
	return g_isHooked;
}

int UnmapDll()
{
	g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)HookProc,
		hDll, GetWindowThreadProcessId(g_hWnd, NULL));

	if (g_hHook == NULL)
		return 0;

	SendMessage(g_hWnd, WM_HOOKEX, 0, 0);

	return !g_isHooked;
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