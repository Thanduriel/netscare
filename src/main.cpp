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

#define PIPES 3

bool firstTime = true;

using namespace std;

void showError(const char* caption, DWORD error);

#pragma data_seg (".shared")
int		g_isHooked = 0;	// START button subclassed?
UINT	WM_HOOKEX = 0;
HWND	g_hWnd = 0;		// handle of START button
HHOOK	g_hHook = 0;
HANDLE	pipeIn;
HANDLE	pipeOut;
#pragma data_seg ()

bool IsValidSwapChain(IDXGISwapChain* pSwapChain)
{
	__try {
		DXGI_FRAME_STATISTICS stats;
		pSwapChain->GetFrameStatistics(&stats);
	}
	__except (1)
	{
		return false;
	}
	return true;
}

bool __stdcall HookD3D11Present(IDXGISwapChain* pSwapChain)
{	
	PresentVTableHook* hook = new PresentVTableHook(pSwapChain);
	auto oldPresent = hook->AddHook(Device::Present, 8);
	Device::SetOrgPresent(oldPresent);

	return true;
}

bool FindSwapChain()
{
	if (!Device::Initialize())
		return false;

	const DWORD_PTR swapChainVTable = reinterpret_cast<DWORD_PTR>(PresentVTableHook::GetVtable(&Device::GetSwapChain()));
	DWORD_PTR dwVTableAddress = 0;
	int objectCount = 0;

	constexpr DWORD_PTR _PTR_MAX_VALUE = 0xFFE00000;
	MEMORY_BASIC_INFORMATION32 mbi = { 0 };
	DWORD fisrAddr = 0x0;
	int count = 0;


	for (DWORD_PTR memptr = 0x10000; memptr < _PTR_MAX_VALUE; memptr = mbi.BaseAddress + mbi.RegionSize) //For x64 -> 0x10000 ->  0x7FFFFFFEFFFF
	{
		if (!VirtualQuery(reinterpret_cast<LPCVOID>(memptr), reinterpret_cast<PMEMORY_BASIC_INFORMATION>(&mbi), sizeof(MEMORY_BASIC_INFORMATION))) //Iterate memory by using VirtualQuery
			break;
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
				IDXGISwapChain* swapChain = reinterpret_cast<IDXGISwapChain*>(current);

				if (swapChain == &Device::GetSwapChain())
					continue;
				else if (IsValidSwapChain(swapChain))
				{
					HookD3D11Present(swapChain);
					return true;
				}
			}
		}
	}

	MessageBox(nullptr, "finished scan", "Caption", MB_OK);
	return false;
}

// **************************************************************** //


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
struct Pipes {
	Pipes(HANDLE hIn, HANDLE hout) : hIn{ hIn }, hOut{ hOut } {}
	HANDLE hIn;
	HANDLE hOut;
};
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
	else if (pCW->message == WM_COPYDATA) {
		MessageBox(NULL, "myEvent", "myEvent", MB_OK);
		PCOPYDATASTRUCT pCDS = reinterpret_cast<PCOPYDATASTRUCT>(pCW->lParam);
		// pipeOut = (HANDLE)(pCDS->dwData);
		Pipes pips = *reinterpret_cast<Pipes*>(pCDS->lpData);
		pipeIn = pips.hIn;
		pipeOut = pips.hOut;
		PVOID pV = pCDS->lpData;
		char meassage[255] = "test3\n1\n";
		DWORD written;
		sprintf_s(meassage, 255, "%d", pCDS->dwData);
		MessageBox(NULL, meassage, "Pipe Handle", MB_OK);
		if (!ReadFile(pipeIn, meassage, 255, NULL, NULL)
			|| !WriteFile(pipeOut, "TestMessage ", 13, &written, NULL)) {
			showError("PipeWriteClient Error", GetLastError());
		} else {
			if(written > 0)
			MessageBox(NULL, "Send data", "Success", MB_OK);
		}
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
COPYDATASTRUCT mCDS;
struct WindowData {
	unsigned long pid;
	HWND hWnd;
};
BOOL isMainHandle(HWND handle) {
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}
BOOL CALLBACK find_window_callback(HWND handle, LPARAM lParam) {
	WindowData& wnd = *reinterpret_cast<WindowData*>(lParam);
	unsigned long pid;
	GetWindowThreadProcessId(handle, &pid);
	if (wnd.pid != pid || !isMainHandle(handle))
		return TRUE;
	wnd.hWnd = handle;
	return FALSE;
}
int InjectDll(HWND hWnd, HANDLE hIn, HANDLE hOut)
{
	pipeIn = hIn;
	pipeOut = hOut;
	g_hWnd = hWnd;

	std::cout << "Hin" << hIn << "\nhOout" << hOut << '\n';

	// Hook
	g_hHook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)HookProc,
		hDll, GetWindowThreadProcessId(hWnd, NULL));
	if (g_hHook == NULL)
		return 0;

	// By the time SendMessage returns, 
	// the START button has already been subclassed
	std::cout << "I" << (LPARAM)hIn << "\nO" << (WPARAM)hOut << "\n";
	Pipes pips{hOut, hIn};
	
	mCDS.dwData = PIPES;
	mCDS.cbData = sizeof(HANDLE);
	mCDS.lpData = &pips;
	WindowData myWnd;
	myWnd.pid = GetCurrentProcessId();
	EnumWindows(find_window_callback, (LPARAM)&myWnd);
	SetWindowTextA(myWnd.hWnd, "Search");
	SetWindowTextA(hWnd, "Target");
	std::cout << "pid:" << myWnd.pid << "\thWnd:" << myWnd.hWnd << '\n';
	SendMessage(hWnd, WM_COPYDATA, (WPARAM)(HWND)myWnd.hWnd, (LPARAM)(LPVOID)&mCDS);
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

/* void showError(const char* caption, DWORD error) {
	LPSTR msg = 0;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		0,
		(LPSTR)&msg, // muss anscheinend so, steht in der Doku
		0,
		NULL) == 0) {
		MessageBox(NULL, "Cant parse Error", caption, MB_OK | MB_ICONERROR);
		return;
	}
	MessageBox(NULL, msg, caption, MB_OK | MB_ICONERROR);
	if (msg) {
		LocalFree(msg);
		msg = 0;
	}
} */