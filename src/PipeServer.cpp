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

/** \attention dem Empfänger kann die Pipe "Weggenommen" werden */
void PipeServer::DualChanelPipe::close() {
	fConnected = false;
	fBroken = true;
	writeP.fBroken = true;
	CloseHandle(writeP.hWrite);
	CloseHandle(writeP.hRead);
	readP.fBroken = true;
	CloseHandle(readP.hWrite);
	CloseHandle(readP.hRead);
}

int PipeServer::closeConnection(std::size_t id) {
	if(!checkId(id)) return -1;
	_pipes[id].close();
	return 0;
}

std::size_t PipeServer::addConnection()
{
	std::size_t id = _pipes.size();
	_pipes.push_back(DualChanelPipe());
	DualChanelPipe &pipe = _pipes[id];
	
	if (pipe.fBroken) return -1;
	return id;
}

bool PipeServer::checkId(std::size_t id) {
	if (id >= _pipes.size()) {
		MessageBox(NULL, "Out of range Accsess", "PipeServer wrong Id", MB_OK | MB_ICONWARNING);
		return false;
	}
	return true;
}

bool PipeServer::checkConnection(std::size_t id) {
	if (!checkId(id))	return false;
	if (_pipes[id].fBroken) return false;
	DualChanelPipe &pipe = _pipes[id];
	// checkConection // TODO:
	pipe.fConnected = true;
	return true;
}
PipeServer::Pipe::Pipe(DWORD bufferSize)
{
	static std::size_t pipeNumber = 0;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	/* if (!CreatePipe(&hRead, &hWrite, &sa, bufferSize)) {
		DWORD error = GetLastError();
		showError("Pipe Creation Failed", error);
		fBroken = true;
		return;
	} */

	char pipeName[255];
	sprintf_s(pipeName, "\\\\.\\Pipe\\NetScare.%08x.%08x", GetCurrentProcessId(), pipeNumber++);
	hRead = CreateNamedPipeA(
		pipeName,
		PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_WAIT,
		1,
		BUFSIZE,
		BUFSIZE,
		120 * 1000,
		&sa
	);
	if (hRead == INVALID_HANDLE_VALUE) {
		showError("Cant Craete Named pipe", GetLastError());
		fBroken = true;
	}

	hWrite = CreateFileA(
		pipeName,
		GENERIC_WRITE,
		0,
		&sa,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL
	);

	if (hWrite == INVALID_HANDLE_VALUE) {
		fBroken = true;
		showError("Cat craete File to read", GetLastError());
	}
	char text[25] = "helle";
	char res[25];
	DWORD length, length2;
	if (!WriteFile(hWrite, text, 25, &length, NULL)) {
		showError("Cant writ ein my Pipe", GetLastError());
		fBroken = true;
		return;
	}
	if (!ReadFile(hRead, res, 25, &length2, NULL)) {
		showError("Cant read form my Pipe", GetLastError());
		fBroken = true;
		return;
	}
	if (length != length2) {
		std::cout << "differenz Message Size\n";
		fBroken = true;
		return;
	}


	std::cout << "buildPipe\n";
	fBroken = false;
}

void PipeServer::printPipe(std::size_t id) {
	if (!checkId(id)) return;
	if (!_pipes[id].fConnected || _pipes[id].fBroken) {
		MessageBox(NULL, "Pipe is Borken or not Connected", "Pipe read error", MB_OK | MB_ICONWARNING);
		return;
	}

	char buf[BUFSIZE+1];
	DWORD readed;
	std::size_t readSum = 0;
	BOOL fSuccess;
	std::cout << "Start´read from Pipe: " << id << ":\n";
	do {
		fSuccess = ReadFile(_pipes[id].readP.hRead, buf, BUFSIZE, &readed, NULL);
		if (fSuccess) {
			buf[readed] = 0;
			std::cout << buf;
		}
		readSum += readed;
	} while (fSuccess && readed == BUFSIZE);
	std::cout << "End read, readed " << readSum << " symbols\n";
}

HANDLE PipeServer::duplicateHandler(PIPE dir, std::size_t id, HANDLE hTargetProcess)
{
	if (!checkId(id)) return INVALID_HANDLE_VALUE;
	HANDLE out;
	HANDLE handle;
	switch (dir) {
	case PIPE_IN:	handle = _pipes[id].writeP.hRead; break;
	case PIPE_OUT:	handle = _pipes[id].readP.hWrite; break;
	}
	std::cout << "handle: " << handle << '\n';
	if (!DuplicateHandle(GetCurrentProcess(), handle, hTargetProcess, &out, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		showError("DuplicationFailed", GetLastError());
		return INVALID_HANDLE_VALUE;
	}
	std::cout << "Handle: " << out << '\n';
	return out;
}
