#include <windows.h>
#include <iostream>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"

HWND hStart;		// handle to start button

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);

int main()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	PipeServer pipeServer{};
	std::size_t pipeId = 0;
	if ((pipeId = pipeServer.addConnection()) == 0) {
		std::cerr << "Can't build Pipe" << '\n';
	}
	si.hStdInput = 0;
	si.hStdOutput = 0;
	std::cout << "Build Pipes" << '\n';
	// Start the child process. 
	std::cout << "PIPH: " << si.hStdOutput << '\n';
	if (!CreateProcess(NULL,   // No module name (use command line)
		"demo.exe",        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		TRUE,          // Set handle inheritance to FALSE
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
	
	WaitForInputIdle(pi.hProcess, 5000);

	HWND hWnd = GetForegroundWindow();
	if (hWnd == nullptr)
		return false;
	DWORD processId;
	GetWindowThreadProcessId(hWnd, &processId);
	HANDLE process = OpenProcess(PROCESS_DUP_HANDLE, TRUE, processId);
	InjectDll(hWnd, pipeServer.duplicateHandler(PipeServer::PIPE_IN, pipeId, process), pipeServer.duplicateHandler(PipeServer::PIPE_OUT, pipeId, process));
	std::cout << "Start connectio  test \n";
	if (!pipeServer.checkConnection(pipeId)) {
		std::cout << "Can't verifeyd the connection" << '\n';
	}
	else {
		std::cout << "Is connected" << '\n';
	}
	char c;
	std::cin >> c;
	char* m = "Hello, World!";
	std::vector<char> res{ 25, 0 };
	WriteTask<char*> wT{m, m+14};
	pipeServer.addTask<char*>(pipeId, wT);
	std::cin >> c;
	pipeServer.printPipe(pipeId);
	std::cin >> c;
	UnmapDll();
//	hStart = ::FindWindow("Shell_TrayWnd", NULL);			// get HWND of taskbar first
//	hStart = ::FindWindowEx(hStart, NULL, "BUTTON", NULL);	// get HWND of start button
	// display main dialog 
//	::DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_WININFO), NULL, MainDlgProc, NULL);

	return 0;
}


//-----------------------------------------------
// MainDlgProc
// Notice: dialog procedure
//
BOOL CALLBACK MainDlgProc(HWND hDlg,	// handle to dialog box
	UINT uMsg,      // message
	WPARAM wParam,  // first message parameter
	LPARAM lParam) // second message parameter
{
/*	static int bChecked = false;

	switch (uMsg) {

	case WM_INITDIALOG:
		::SetTimer(hDlg, 101, 300, NULL);
		return true;

		// if running more instances of HookInjEx, 
		// keep their interface consistent
	case WM_TIMER:
		bChecked = (IsDlgButtonChecked(hDlg, IDC_BUTTON) == BST_CHECKED);
		if (g_bSubclassed && !bChecked) {
			::CheckDlgButton(hDlg, IDC_BUTTON, BST_CHECKED);
			::SetDlgItemText(hDlg, IDC_BUTTON, "Unmap Dll");
		}
		else if (!g_bSubclassed && bChecked) {
			::CheckDlgButton(hDlg, IDC_BUTTON, BST_UNCHECKED);
			::SetDlgItemText(hDlg, IDC_BUTTON, "Inject Dll");
		}
		break;

	case WM_COMMAND:
		if (!g_bSubclassed) {
			InjectDll(hStart);
			if (g_bSubclassed)
				::SetDlgItemText(hDlg, IDC_BUTTON, "Unmap Dll");
		}
		else {
			UnmapDll();
			if (!g_bSubclassed)
				::SetDlgItemText(hDlg, IDC_BUTTON, "Inject Dll");
		}
		break;

	case WM_CLOSE:
		if (g_bSubclassed)
			UnmapDll();

		::EndDialog(hDlg, 0);
		break;
	}*/

	return false;
}