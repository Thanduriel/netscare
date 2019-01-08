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

void PipeServer::DualChanelPipe::close() {
	fClosed = true;
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
	_pipes[id - 1].close();
	return 0;
}

std::size_t PipeServer::addConnection(STARTUPINFO &si)
{
	std::size_t id = _pipes.size();
	_pipes.push_back(DualChanelPipe());
	DualChanelPipe &pipe = _pipes[id];
	
	if (pipe.fBroken) return 0;
	si.hStdInput = pipe.writeP.hRead;
	si.hStdOutput = pipe.readP.hWrite;
	si.dwFlags |= STARTF_USESTDHANDLES;

	return id +1;
}

bool PipeServer::checkId(std::size_t id) {
	if (id == 0 || id > _pipes.size()) {
		MessageBox(NULL, "Out of range Accsess", "PipeServer wrong Id", MB_OK | MB_ICONWARNING);
		return false;
	}
	return true;
}

bool PipeServer::checkConnection(std::size_t id) {
	if (!checkId(id))	return false;
	if (_pipes[id - 1].fBroken) return false;
	DualChanelPipe &pipe = _pipes[id - 1];
	// checkConection // TODO:
	pipe.fConnected = true;
	return true;
}
PipeServer::Pipe::Pipe(DWORD bufferSize)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&hRead, &hWrite, &sa, bufferSize)) {
		DWORD error = GetLastError();
		showError("Pipe Creation Failed", error);
		fBroken = true;
		return;
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
	if (!_pipes[id - 1].fConnected || _pipes[id - 1].fBroken) {
		MessageBox(NULL, "Pipe is Borken or not Connected", "Pipe read error", MB_OK | MB_ICONWARNING);
		return;
	}

	char buf[BUFSIZE+1];
	DWORD readed;
	std::size_t readSum = 0;
	BOOL fSuccess;
	std::cout << "Start´read from Pipe: " << id << ":\n";
	do {
		fSuccess = ReadFile(_pipes[id - 1].readP.hRead, buf, BUFSIZE, &readed, NULL);
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
	case PIPE_IN:	handle = _pipes[id - 1].writeP.hRead; break;
	case PIPE_OUT:	handle = _pipes[id - 1].readP.hWrite; break;
	}
	std::cout << "handle: " << handle << '\n';
	if (!DuplicateHandle(GetCurrentProcess(), handle, hTargetProcess, &out, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		showError("DuplicationFailed", GetLastError());
		return INVALID_HANDLE_VALUE;
	}
	std::cout << "Handle: " << out << '\n';
	return out;
}
