#pragma once

#include <string>
#include <vector>
#include <Windows.h>

class PipeServer
{
public:
	PipeServer();
	~PipeServer();
	/** \return pipeId \param si startInformation for Child Process */
	std::size_t addConnection(STARTUPINFO &si);
	/** \return exitcode: 0:success, 1:already closed, -1:error*/
	int closeConnection(std::size_t id);
	bool checkConnection(std::size_t id);
private:
	struct Pipe {
		Pipe(DWORD bufferSize = 0);
		HANDLE hRead;
		HANDLE hWrite;
		BOOL fBroken;
	};
	struct DualChanelPipe {
		DualChanelPipe(DWORD bufferSize = 0)
			: readP{ bufferSize },
			writeP{ bufferSize },
			fConnected{ false }
		{
			fBroken = readP.fBroken || writeP.fBroken;
		}
		Pipe readP, writeP;
		BOOL fConnected, fBroken;
	};
	std::vector<DualChanelPipe> _pipes;
};

