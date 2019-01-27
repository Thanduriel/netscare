#include <windows.h>
#include <iostream>
#include <map>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"
#include "../server/Asn.hpp"
#include "Network.hpp"
#include "Gui.hpp"
#include "scanner.hpp"

HWND hStart;		// handle to start button

// #define RUN_DEMO


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

#ifdef RUN_DEMO
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
	HWND hWnd = GetForegroundWindow();
	if (hWnd == nullptr)
		return false;
	DWORD processId;
	GetWindowThreadProcessId(hWnd, &processId);
	HANDLE process = OpenProcess(PROCESS_DUP_HANDLE, TRUE, processId);
	
	InjectDll(hWnd, pipeServer.duplicateHandler(PipeServer::PIPE_IN, pipeId, process), pipeServer.duplicateHandler(PipeServer::PIPE_OUT, pipeId, process));
#else
	/* ProcessScanner scanner;
	auto it = std::find_if(scanner.begin(), scanner.end(), [](const ProcessInfo& _info) 
	{
		return _info.name.find("Tropico") != std::string::npos;
	});

	if (it != scanner.end())
	{
		HANDLE process = OpenProcess(PROCESS_DUP_HANDLE, TRUE, it->ID);
		InjectDll(Utils::GetProcessWindow(it->ID), pipeServer.duplicateHandler(PipeServer::PIPE_IN, pipeId, process), pipeServer.duplicateHandler(PipeServer::PIPE_OUT, pipeId, process));
	}
	else return 1; */
#endif

	if (!pipeServer.checkConnection(pipeId)) {
		std::cout << "Can't verifeyd the connection" << '\n';
	}
	else {
		std::cout << "Is connected" << '\n';
	}

	struct TaskQueue {
		Action::TYPE type;
		std::unique_ptr<Task<unsigned char*>> task;
		TaskQueue(const Action& action, std::unique_ptr<Task<unsigned char*>>task) : type{ action.type }, task { std::move(task) } {}
	};
	std::vector<Address> addresses;
	std::vector<TaskQueue> actions;
	std::size_t pos = 0;
	unsigned char _msg[] = {255, 255, 0, 255};

	std::vector<ScareEventCp> events;
	std::string arg;
	int port = 80;
	wchar_t *username = nullptr;
	{
		std::string str = GetCommandLine();
		int pos = str.find(' ');
		while (str[++pos] == ' ');
		int end = str.find(' ', pos);
		if (end == std::string::npos) arg = str.substr(pos);
		else {
			while (str[++end] == ' ');
			arg = str.substr(pos, end - pos - 1);
		}
		// MessageBoxA(NULL, ("a"+arg+"b").c_str(), std::to_string(strlen(arg.c_str())).c_str(), MB_OK);
	}
	Client client(addresses, arg.c_str(), 9090);
	Gui gui(hInst, addresses);
	clock_t start = clock();
	bool run = true;
	while (run) {
		if (pos < actions.size()) {
			Task<unsigned char*>::STATUS_CODE sc = actions[pos].task->getState();
			if (sc == Task<unsigned char*>::SUCCESS) {
				actions.erase(actions.begin() + pos);
				memset(&actions[pos], 0, sizeof(TaskQueue));
				++pos;
			}
			else if (sc == Task<unsigned char*>::FAILED || sc == Task<unsigned char*>::ERROR_TO_SHORT) ++pos;
		}

		Action action = gui.update();
		switch (action.type) {
		case Action::LOGIN: {
			if (client.IsOnline()) break;
			const wchar_t *uName = reinterpret_cast<const wchar_t*>(action.data);
			if (username) delete[] username;
			unsigned long len = wcslen(uName) + 1;
			username = new wchar_t[len];
			wcscpy_s(username, len, uName);
			if (!client.Login(username)) { delete[] username; username = nullptr; }
			else gui.SetUserName(username);
			break;
		}
		case Action::EV_SETUP: {
			ScareEvent * scareEv = reinterpret_cast<ScareEvent*>(action.data);
			events.emplace_back( scareEv );
			client.AddEvent(events.back());
		}	break;
		case Action::EV_UPADTE: {
			int eventId = *reinterpret_cast<int*>(action.data);
			for (ScareEventCp& e : events) {
				if (e.id == eventId) {
					// client.SendPicture(e); TODO: stuff
					break;
				}
			}
		}	break;
		case Action::EV_TRIGGER: {
			int eId = *reinterpret_cast<int*>(action.data);
			client.TriggerEvent(eId);
			for (ScareEventCp& ev : events) {
				if (ev.id == eId) {
					ev.changeState(ScareEvent::TRIGGERD);
				}
			}

			/* int id = *reinterpret_cast<int*>(action.data);
			for (auto itr = events.begin(); itr != events.end(); ++itr) {
				if (itr->id == id && itr->isValid()) {
					itr->changeState(ScareEvent::EXECUTED);
					addresses[id].tickets -= 1;
					itr->close();
					MessageBox(NULL, (std::to_string(itr->target) + "  " + itr->file).c_str(), "Action", MB_OK);
					break;
				}
			}*/
		}	break;
		case Action::SETCOLOR:
			actions.push_back(TaskQueue(
				std::move(action),
				std::make_unique<WriteTask<unsigned char*>>(action.begin(), action.end())
			));
			pipeServer.addTask(pipeId, *actions.back().task);
			break;
		case Action::CLOSE:
			run = false;
			break;
		}
		if (((clock() - start) / CLOCKS_PER_SEC) > 1.f) {
			client.Update();
			start = clock();
		}
	}

	UnmapDll();
	return 0;
}
