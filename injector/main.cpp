#include <windows.h>
#include <iostream>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"
#include "../src/Gui.hpp"

HWND hStart;		// handle to start button


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

	Gui gui(hInst);

	struct TaskQueue {
		Action::TYPE type;
		Task<unsigned char*>& task;
		TaskQueue(const Action& action, Task<unsigned char*>& task) : type{ action.type }, task{ task } {}
	};

	std::vector<TaskQueue> actions;
	unsigned char _msg[] = {255, 255, 0, 255};

	bool run = true;
	while (run) {
		Action action = gui.update();
		switch (action.type) {
		case Action::SETCOLOR:
			actions.push_back(TaskQueue(
				action,
				WriteTask<unsigned char*>(action.begin(), action.end())
			));
			pipeServer.addTask(pipeId, actions.back().task);
			break;
		case Action::CLOSE:
			run = false;
			break;
		}
	}

	UnmapDll();
	return 0;
}
