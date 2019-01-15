#include <windows.h>
#include <iostream>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"

HWND hStart;		// handle to start button

LRESULT CALLBACK MainWinProc(HWND, UINT, WPARAM, LPARAM);

static void showError(const char* caption, DWORD error) {
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
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE prevInstanc, LPSTR args, int ncmdshow)
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
	char msg[] = "Hello, World!\n";
	char ms[17];
	ms[0] = 'S';
	ms[16] = 0;
	HWND hWnd = GetForegroundWindow();
	if (hWnd == nullptr)
		return false;
	DWORD processId;
	GetWindowThreadProcessId(hWnd, &processId);
	HANDLE process = OpenProcess(PROCESS_DUP_HANDLE, TRUE, processId);
	WriteTask<char*> wT(msg, msg + 16);
	std::cout << "Hello: " << *msg << '\t' << *ms << '\n';
	ReadTask<char*> rT(ms, 16);
	pipeServer.addTask(pipeId, wT);
	if (wT.getState(TRUE) == Task<char*>::FAILED) {
		std::cerr << "Failed To Use Pipe\n";
	}
	pipeServer.addTask(pipeId, rT);
	InjectDll(hWnd, pipeServer.duplicateHandler(PipeServer::PIPE_IN, pipeId, process), pipeServer.duplicateHandler(PipeServer::PIPE_OUT, pipeId, process));
	std::cout << "Start connectio  test \n";
	if (!pipeServer.checkConnection(pipeId)) {
		std::cout << "Can't verifeyd the connection" << '\n';
	}
	else {
		std::cout << "Is connected" << '\n';
	}
	if (rT.getState(TRUE) != Task<char*>::STATUS_CODE::SUCCESS) {
		std::cout << "Failed to Pipe\n";
	} else {
		std::cout << "readdaMsg:\n" << ms << " - length - " << rT.rededBytes() << '\n';
	}

	WNDCLASSW winC = { 0 };
	winC.hCursor = LoadCursor(NULL, IDC_ARROW);
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"MainWindow";
	winC.lpfnWndProc = MainWinProc;

	if (!RegisterClassW(&winC)) {
		std::cerr << "Can't register winC\n";
		UnmapDll();
		return -1;
	}

	HWND hINPUT = CreateWindowW(L"MainWindow", L"NetScare", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 500, NULL, NULL, NULL, NULL);

	MSG winMsg = { 0 };
	while (GetMessage(&winMsg, NULL, NULL, NULL)) {
		TranslateMessage(&winMsg);
		if (winMsg.message == WM_KEYDOWN && winMsg.wParam == VK_RETURN)
			SendMessageW(hINPUT, winMsg.message, winMsg.wParam, winMsg.wParam);
		else
			DispatchMessageW(&winMsg);
	}

	UnmapDll();
	return 0;
}

void addUI(HWND hWnd, HWND& hColor) {
	CreateWindowW(L"static", L"Color(r,g,b): ", WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, hWnd, NULL, NULL, NULL);
	hColor = CreateWindowW(L"edit", L"255,255,255", WS_VISIBLE | WS_CHILD, 110, 10, 100, 20, hWnd, NULL, NULL, NULL);
}

LRESULT CALLBACK MainWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static HWND hColor;
	switch (msg) {
	case WM_CREATE:
		addUI(hWnd, hColor);
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		default:
			break;
		}
		break;
	case WM_KEYDOWN:
		
		switch (wParam) {
		case VK_RETURN:
		{
			wchar_t text[32];
			if (!GetWindowTextW(hColor, text, 32)) {
				showError("SetError", GetLastError());
			}
			SetWindowTextW(hWnd, text);
		}
		break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
		return 0;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
}