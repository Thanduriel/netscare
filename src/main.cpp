#include <windows.h>
#include <stdio.h>
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
	MessageBox(
		NULL,
		(LPCSTR)"Resource not available\nDo you want to try again?",
		(LPCSTR)"Account Details",
		MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2
	);
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
	// DetourTransactionBegin();
	// DetourUpdateThread(GetCurrentThread());
	// DetourAttach(reinterpret_cast<PVOID*>(reinterpret_cast<DWORD*>(pSwapChain) + 8), Present);
	// reinterpret_cast<DWORD*>(pSwapChain)[8] = (DWORD)Present;
	//(*reinterpret_cast<DWORD**>(pSwapChain))[8] = (DWORD)Present;
	// (*((DWORD_PTR**)pSwapChain))[8] = (DWORD_PTR)Present;
	int dwIndex = 0;
	PDWORD pdwVMT = *(reinterpret_cast<DWORD_PTR**>(pSwapChain));
	for (dwIndex = 0; pdwVMT[dwIndex]; ++dwIndex) {
		if (IsBadCodePtr((FARPROC)pdwVMT[dwIndex]))
		{
			break;
		}
	}
	std::cout << "Size " << dwIndex << std::endl;
	MEMORY_BASIC_INFORMATION memInfo;
	SIZE_T size;
	VirtualQuery(pdwVMT, &memInfo, 8 * sizeof(DWORD));
	std::cout << "memInfo:\nBaseAddr\t" << memInfo.BaseAddress << "\nAllocBase\t" << memInfo.AllocationBase << "\nAllocProtecte\t" << memInfo.AllocationProtect << "\nSize\t"
		<< memInfo.RegionSize << "\nState\t" << memInfo.State << "\nProtect\t" << memInfo.Protect << "\nType\t" << memInfo.Type << "\n";
	// pdwVMT[8] = (DWORD)Present;
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
					cout << "\nFound It\n" << endl;
					return hookD3D11Present((IDXGISwapChain*)current);    //If found we hook Present in swapChain
				}
			}
		}
	}
	return true;
}



#include <Windows.h>
#include <iostream>
#include <string>
#include <tlhelp32.h>
#include <Psapi.h>
#include <tchar.h>

int getPID(const std::string& name);
DWORD64 GetModule(HANDLE hProcess, const std::string& name);

// Stolen from: https://docs.microsoft.com/en-gb/windows/desktop/SecAuthZ/enabling-and-disabling-privileges-in-c--
BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		printf("LookupPrivilegeValue error: %u\n", GetLastError());
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
		printf("AdjustTokenPrivileges error: %u\n", GetLastError());
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		printf("The token does not have the specified privilege. \n");
		return FALSE;
	}

	return TRUE;
}


constexpr const char* theProcess = "demo.exe";


int getPID(const std::string& name)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (!Process32First(snapshot, &entry)) return NULL;

	do
	{
		if (strcmp(entry.szExeFile, name.c_str()) == 0)
		{
			CloseHandle(snapshot);
			return entry.th32ProcessID;
		}
	} while (Process32Next(snapshot, &entry));

	CloseHandle(snapshot);
	return NULL;
}

DWORD64 GetModule(HANDLE hProcess, const std::string& name)
{
	HMODULE hMods[1024];
	DWORD cbNeeded;

	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH];
			if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
			{
				std::string modName = szModName;
				if (modName.find(name) != std::string::npos)
				{
					return (DWORD64)hMods[i];
				}
			}
		}
	}
	return NULL;
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
	printf("Create Process succsess");

	HANDLE hToken;
	BOOL ok = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	if (!ok)
	{
		std::cout << "OpenProcessToken failed, error " << GetLastError() << "\n";
		return 255;
	}

	ok = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
	if (!ok)
	{
		CloseHandle(hToken);
		return 1;
	}

	int pid = getPID(theProcess);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL)
	{
		std::cout << "OpenProcess failed, error " << GetLastError() << "\n";
		CloseHandle(hToken);
		return 1;
	}

	DWORD64 baseAddress = GetModule(hProcess, theProcess);
	std::cout << "Base Address: " << std::hex << std::uppercase << "0x" << baseAddress << "\n";
	int buffer = 0;  // Note: sizeof (buffer) below, not sizeof (&buffer)
	ok = ReadProcessMemory(hProcess, (LPCVOID)baseAddress, (LPVOID)&buffer, sizeof(buffer), NULL);
	if (ok)
	{
		std::cout << "ReadProcessMemory succeeded, buffer = " << buffer << "\n";
		system("pause");
		return 0;
	}
	else {
		std::cout << "Read Errir, error " << GetLastError() << "\n";
		system("pause");
	}
	ok = WriteProcessMemory(hProcess, (LPVOID)baseAddress, (LPVOID)&buffer, sizeof(buffer), NULL);

	if (ok)
	{
		std::cout << "Write succeeded\n";
		system("pause");
		return 0;
	}
	else {
		std::cout << "Write Errir, error " << GetLastError() << "\n";
		system("pause");
	}

	FindSwapChain();

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode;
	BOOL result = GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(hProcess);
	CloseHandle(hToken);
	
	if (!result) {
		printf("Can't get exitCode from Process\n");
		return 1;
	}
	printf("ExitCode: %d", exitCode);
	return 0;
}