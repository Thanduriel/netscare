#include <windows.h>
#include <iostream>
#include <map>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"
#include "Gui.hpp"

HWND hStart;		// handle to start button

void showEror(const char* caption, DWORD error) {
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

	Sleep(500);
	
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

	struct TaskQueue {
		Action::TYPE type;
		Task<unsigned char*>& task;
		TaskQueue(const Action& action, Task<unsigned char*>& task) : type{ action.type }, task{ task } {}
	};
	std::vector<Address> addresses;
	addresses.push_back(Address(L"Jörgen", 1, 1));
	addresses.push_back(Address(L"YOLO", 2, 8));
	addresses.push_back(Address(L"HanHan", 23, 1));
	std::vector<TaskQueue> actions;
	std::size_t pos = 0;
	unsigned char _msg[] = {255, 255, 0, 255};

	std::vector<ScareEventCp> events;
	Gui gui(hInst, addresses);

	bool run = true;
	while (run) {
		if (pos < actions.size()) {
			Task<unsigned char*>::STATUS_CODE sc = actions[pos].task.getState();
			if (sc == Task<unsigned char*>::SUCCESS) {
				actions[pos].~TaskQueue();
				memset(&actions[pos], 0, sizeof(TaskQueue));
				++pos;
			}
			else if (sc == Task<unsigned char*>::FAILED || sc == Task<unsigned char*>::ERROR_TO_SHORT) ++pos;
		}
		Action action = gui.update();
		switch (action.type) {
		case Action::EV_SETUP: {
			ScareEvent * scareEv = reinterpret_cast<ScareEvent*>(action.data);
			/* MessageBox(NULL,
				(std::to_string(scareEv->target) + "  " + scareEv->file).c_str(),
				"SetUpEvent", MB_OK); */
			events.emplace_back( scareEv );
			events.back().changeState(ScareEvent::SETTET);
			// check if data already exiist 
			// check if user has data or is already pending
			// delete action.data; ?? dont work
			// check if already setet
			// ConvertIamge to DDS
			// save Path and convertet for later
			// send dds to Person
			// on success set addressState state Set
			// MessageBox(NULL, std::to_string(events.back().id).c_str(), "Last id", MB_OK);
		}	break;
		case Action::EV_UPADTE:
			// same as SetUp but withput set referens
			break;
		case Action::EV_TRIGGER: {
			int id = *reinterpret_cast<int*>(action.data);
			for (auto itr = events.begin(); itr != events.end(); ++itr) {
				if (itr->id == id && itr->isValid()) {
					itr->changeState(ScareEvent::EXECUTED);
					addresses[id].tickets -= 1;
					itr->close();
					MessageBox(NULL, (std::to_string(itr->target) + "  " + itr->file).c_str(), "Action", MB_OK);
					break;
				}
			}
				// check if dds path setet
				// send trigger
				// clear file
		}	break;
		case Action::SETCOLOR:
			actions.push_back(TaskQueue(
				std::move(action),
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
