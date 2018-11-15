#include "../src/PipeServer.hpp"
#include <iostream>

constexpr std::size_t BUFSIZE = 512;
void showError(const char* caption, DWORD error) {
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


PipeServer::PipeServer()
{}


PipeServer::~PipeServer()
{}


int PipeServer::closeConnection(std::size_t id) {
	MessageBox(NULL, "Not implemented yet", "Can't close Connection", MB_OK | MB_ICONWARNING);
	return -1;
}

std::size_t PipeServer::addConnection(STARTUPINFO &si)
{
	std::size_t id = _pipes.size();
	_pipes.push_back(DualChanelPipe());
	DualChanelPipe &pipe = _pipes[id];
	
	if (pipe.fBroken) return 0;
	si.hStdInput = pipe.writeP.hRead;
	si.hStdOutput = pipe.readP.hWrite;

	return id +1;
}

bool PipeServer::checkConnection(std::size_t id) {
	if (id == 0 || id > _pipes.size()) {

		return false;
	}
	DualChanelPipe &pipe = _pipes[id - 1];

	char hello[25] = "Hello ";
	DWORD written;
	std::cout << "StartWrite\n";
	if (!WriteFile(pipe.writeP.hWrite, hello, 25, &written, NULL)) {
		DWORD error = GetLastError();
		showError("Pipe send test Failed", error);
		return false;
	}
	std::cout << "Finfished Write\n";
	if (!ReadFile(pipe.readP.hRead, hello, 25, &written, NULL)) {
		std::cout << "Get from Child: " << hello << '\n';
	}
	else {
		DWORD error = GetLastError();
		showError("Pipe read test Failed", error);
		return false;
	}

	pipe.fConnected = true;
	return true;
}
PipeServer::Pipe::Pipe(DWORD bufferSize)
{
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	if (!CreatePipe(&hRead, &hWrite, &sa, bufferSize)) {
		DWORD error = GetLastError();
		showError("Pipe Creation Failed", error);
		fBroken = true;
		return;
	}
	fBroken = false;
}
