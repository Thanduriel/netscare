#include <windows.h>
#include <iostream>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"

HWND hStart;		// handle to start button

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

constexpr int WM_SETCOLOR = WM_USER + 1; // wParam = rgba8

constexpr int COM_SETCOLOR = 1;
constexpr int COM_ADDQUEUE = 2;

LRESULT CALLBACK MainWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueWindowProc(HWND, UINT, WPARAM, LPARAM);

struct MainWinState {
	HWND hColorE{ 0 }, hQueueBox{0};
	std::vector<HWND> hQueues{0};
};

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
	HWND hINPUT;
	{
		MainWinState *pState = new (std::nothrow) MainWinState{0};
		hINPUT = CreateWindowW(L"MainWindow", L"NetScare", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 500, NULL, NULL, NULL, pState);
	}

	MSG winMsg = { 0 };
	
	std::vector<WriteTask<std::vector<unsigned char>::iterator, std::vector<unsigned char>>> wQueue{};
	auto wPos = wQueue.begin();
	if (wPos != wQueue.end())
		MessageBox(NULL, "NOOOO", "Failed", MB_OK | MB_ICONERROR);

	while (GetMessage(&winMsg, NULL, NULL, NULL)) {
		TranslateMessage(&winMsg);
		switch (winMsg.message) {
		case WM_KEYDOWN:
			switch (winMsg.wParam) {
			case VK_RETURN:
				SendMessageW(hINPUT, winMsg.message, winMsg.wParam, (LPARAM)winMsg.hwnd);
				break;
			default:
				DispatchMessageW(&winMsg);
			}
			break;
		case WM_SETCOLOR: {
			std::vector<unsigned char> rgba(4);
			*reinterpret_cast<WPARAM*>(rgba.data()) = winMsg.wParam;
			bool empty = wPos == wQueue.end();
			wQueue.push_back(WriteTask<std::vector<unsigned char>::iterator, std::vector<unsigned char>>(std::move(rgba)));
			if (empty)
				wPos = --wQueue.end();
			pipeServer.addTask(pipeId, wQueue.back());
		}
		break;
		default:
			DispatchMessageW(&winMsg);
		}

		if (wPos != wQueue.end()) {
			auto sc = wPos->getState();
			if (sc != wPos->PENDING) {
				if (sc == wPos->FAILED)
					MessageBox(NULL,"WriteTaskFailed", "Task Failed", MB_OK | MB_ICONERROR);
				++wPos;
			}
		}
	}

	UnmapDll();
	return 0;
}

void addUI(HWND hWnd, MainWinState* pState) {
	CreateWindowW(L"static", L"Color(r,g,b): ", WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, hWnd, NULL, NULL, NULL);
	pState->hColorE = CreateWindowW(L"edit", L"255,255,255", WS_VISIBLE | WS_CHILD, 110, 10, 100, 20, hWnd, NULL, NULL, NULL);
	CreateWindowW(L"button", L"set bg color", WS_VISIBLE | WS_CHILD, 10, 60, 100, 20, hWnd, (HMENU)COM_SETCOLOR, NULL, NULL);
	CreateWindowW(L"button", L"AddQueue", WS_VISIBLE | WS_CHILD, 110, 60, 100, 20, hWnd, (HMENU)COM_ADDQUEUE, NULL, NULL);
	pState->hQueueBox = CreateWindowW(L"static", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_HSCROLL, 10, 110, 400, 300, hWnd, NULL, NULL, NULL);
}

LRESULT CALLBACK MainWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	MainWinState *pState = reinterpret_cast<MainWinState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_CREATE: {
		CREATESTRUCTW *pCreat = reinterpret_cast<CREATESTRUCTW*>(lParam);
		MainWinState *pStateC = reinterpret_cast<MainWinState*>(pCreat->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pStateC);
		pState = pStateC;
		addUI(hWnd, pState);
	}
	break;
	case WM_COMMAND:
		switch (wParam) {
		default: com = wParam;
		}
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			if (lParam == (LPARAM)pState->hColorE) com = COM_SETCOLOR;
		break;
		}
		break;
	case WM_DESTROY:
		delete pState;
		PostQuitMessage(0);
		break;
		return 0;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	switch (com) {
	case COM_SETCOLOR: {
		wchar_t text[32];
		GetWindowTextW(pState->hColorE, text, 32);
		unsigned char rgba[4] = { 0, 0, 0, 0 };
		rgba[3] = 255;
		swscanf_s(text, L"%hhu, %hhu, %hhu", rgba, rgba + 1, rgba + 2);
		PostMessageW(NULL, WM_SETCOLOR, *reinterpret_cast<WPARAM*>(rgba), NULL);
	}
	break;
	case COM_ADDQUEUE: {
		pState->hQueues.push_back(CreateWindowW(L"static", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
			5 + 55 * pState->hQueues.size(), 5, 50, 290, pState->hQueueBox, NULL, NULL, NULL));
	} break;
	}
}

LRESULT CALLBACK QueueWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	switch (msg) {
	default: return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	switch (com) {}
}