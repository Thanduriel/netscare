#include <windows.h>
#include <stdio.h>
#include <detours.h>
#include <d3d.h>
#include <d3d11.h>
#include <iostream>

typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
D3D11PresentHook D3D11Present = nullptr;


ID3D11Device* m_pDevice = nullptr;
ID3D11DeviceContext* m_pContext = nullptr;
IDXGISwapChain* m_pSwapChain = nullptr;

DWORD_PTR* m_pSwapChainVtable = NULL;
DWORD_PTR dwVTableAddress = NULL;

D3D11PresentHook pPresent = NULL;

bool firstTime = true;

void DrawString(char* text, float size, int x, int y, int color)
{
	//CONVERTING CHAR* TO CONST WCHAR*
	const WCHAR *pwcsName;
	int nChars = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
	pwcsName = new WCHAR[nChars];
	MultiByteToWideChar(CP_ACP, 0, text, -1, (LPWSTR)pwcsName, nChars);
}

using namespace std;

HRESULT __stdcall Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	std::cout << "test\n";
	if (firstTime)
	{
		__asm
		{
			push eax
			mov eax, [ebp + 8]
			mov This, eax
			pop eax
		}

		//GET DEVICE
		This->GetDevice(__uuidof(ID3D11Device), (void**)&m_pDevice);

		//CHECKING IF DEVICE IS VALID
		cout << "New m_pDevice:			0x" << hex << m_pDevice << endl;
		if (!m_pDevice) return false;

		//REPLACING CONTEXT
		m_pDevice->GetImmediateContext(&m_pContext);

		//CHECKING IF CONTEXT IS VALID
		cout << "New m_pContext:			0x" << hex << m_pContext << endl;
		if (!m_pContext) return false;

		cout << "" << endl;

		m_pDevice->Release();
		cout << "m_pDevice released" << endl;

		m_pContext->Release();
		cout << "m_pContext released" << endl;

		m_pSwapChain->Release();
		cout << "m_pSwapChain released" << endl;

		firstTime = false;
	}
	DrawString("DirectX Hook", 70, 20, 20, 0xffff1612);
	return pPresent(This, SyncInterval, Flags);
}

bool __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain)
{
	cout << "Hooking Present" << endl;
	cout << "" << endl;

	cout << "New m_SwapChain:		0x" << hex << pSwapChain << endl;
	//CHECKING IF SWAPCHAIN IS VALID
	if (!pSwapChain) return false;

	//HOOKING PRESENT
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(reinterpret_cast<PVOID*>(reinterpret_cast<DWORD*>(pSwapChain) + 8), Present);
	auto error = DetourTransactionCommit();
//	CVMTHook* VMTSwapChainHook = new CVMTHook((DWORD_PTR**)pSwapChain);
//	pPresent = (D3D11PresentHook)VMTSwapChainHook->HookMethod((DWORD_PTR)Present, 8);

	cout << "pPresent:			0x" << hex << pPresent << endl;
	//CHECKING IF PRESENT IS VALID
	if (!pPresent) return false;
	return true;
}

bool __stdcall CreateDeviceAndSwapChain()
{
	system("Color 2");
	cout << "Creating Device & Swapchain" << endl;


	HWND hWnd = GetForegroundWindow();
	if (hWnd == nullptr)
		return false;


	cout << "" << endl;
	cout << "HWND:				0x" << hex << hWnd << endl;


	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = (GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) != 0 ? FALSE : TRUE;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &m_pSwapChain, &m_pDevice, NULL, &m_pContext)))
	{
		MessageBox(hWnd, "Unable to create pDevice & pSwapChain!", "FATAL ERROR", MB_ICONERROR);
		return false;
	}


	cout << "m_pSwapChain:			0x" << hex << m_pSwapChain << endl;
	cout << "m_pDevice:			0x" << hex << m_pDevice << endl;
	cout << "m_pContext:			0x" << hex << m_pContext << endl;

	m_pSwapChainVtable = (DWORD_PTR*)m_pSwapChain;
	m_pSwapChainVtable = (DWORD_PTR*)m_pSwapChainVtable[0];

	cout << "m_pSwapChainVtable:		0x" << hex << m_pSwapChainVtable << endl;

	DWORD dwOld;
	VirtualProtect(pPresent, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	return (m_pSwapChainVtable);
}

bool FindSwapChain()
{
	if (CreateDeviceAndSwapChain() == false)
		return false;

#define _PTR_MAX_VALUE 0xFFE00000
	MEMORY_BASIC_INFORMATION32 mbi = { 0 };


	for (DWORD_PTR memptr = 0x10000; memptr < _PTR_MAX_VALUE; memptr = mbi.BaseAddress + mbi.RegionSize) //For x64 -> 0x10000 ->  0x7FFFFFFEFFFF
	{
		if (!VirtualQuery(reinterpret_cast<LPCVOID>(memptr), reinterpret_cast<PMEMORY_BASIC_INFORMATION>(&mbi), sizeof(MEMORY_BASIC_INFORMATION))) //Iterate memory by using VirtualQuery
			continue;

		if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || mbi.Protect & PAGE_GUARD) //Filter Regions
			continue;

		DWORD_PTR len = mbi.BaseAddress + mbi.RegionSize;     //Do once

		for (DWORD_PTR current = mbi.BaseAddress; current < len; ++current)
		{
			__try
			{
				dwVTableAddress = *(DWORD_PTR*)current;
			}
			__except (1)
			{
				continue;
			}

			if (dwVTableAddress == (DWORD_PTR)m_pSwapChainVtable)
			{
				if (current == (DWORD_PTR)m_pSwapChain)
					continue;
				else
				{
					cout << "" << endl;
					return hookD3D11Present((IDXGISwapChain*)current);    //If found we hook Present in swapChain
				}
			}
		}
	}
	return true;
}

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

	FindSwapChain();

	return 0;
}