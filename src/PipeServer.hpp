#pragma once

#include <string>
#include <vector>
#include <Windows.h>

class PipeServer
{
public:
	enum PIPE {PIPE_IN, PIPE_OUT};
	PipeServer();
	~PipeServer();
	/** \return pipeId \param si startInformation for Child Process */
	std::size_t addConnection(STARTUPINFO &si);
	/** \return exitcode: 0:success, 1:already closed, -1:error*/
	int closeConnection(std::size_t id);
	bool checkConnection(std::size_t id);
	void printPipe(std::size_t id);
	HANDLE duplicateHandler(PIPE dir, std::size_t id, HANDLE hTargetProcess);
private:
	enum Commands {
		PIPE_CLOSE = 0x01
	};
	struct Pipe {
		Pipe(DWORD bufferSize = 0);
		HANDLE hRead;
		HANDLE hWrite;
		BOOL fBroken;
	};
	bool checkId(std::size_t id);
	struct DualChanelPipe {
		DualChanelPipe(DWORD bufferSize = 0)
			: readP{ bufferSize },
			writeP{ bufferSize },
			fConnected{ false }
		{
			fBroken = readP.fBroken || writeP.fBroken;
		}
		void close();
		Pipe readP, writeP;
		BOOL fConnected, fBroken, fClosed;
	};
	std::vector<DualChanelPipe> _pipes;
};

