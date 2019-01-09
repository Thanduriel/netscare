#pragma once

#include <string>
#include <vector>
#include <Windows.h>

/** check for itretaor type https://isocpp.org/files/papers/N3911.pdf */
/* template<typename...>
using void_t = void;

template<typename T, typename = void>
struct is_iterator : std::flase_type {};

template <typename T>
struct is_itrator<T,
	void_t<typename std::iterator_traits<T>::iterator_category>>
	: std::true_type {}; */

template <typename T> /**< char random access iterator*/
class Task {
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

public:
	enum TASK_TYPE {READ, WRITE};
	enum STATUS_CODE { SUCCESS, NOT_STARTED, PENDING, FAILED, ERROR_TO_SHORT};
	Task(const TASK_TYPE type, T begin, T end) : type{ type }, pipe{ 0 }, begin{ begin }, end{ end } {
		overlap.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		if (!overlap.hEvent) {
			showError("Create EventHandler failed!", GetLastError());
		}
		length = 0;
		fPending = false;
		bStarted = false;
		bFailed = false;
	}
	void setPipeHandle(HANDLE pipe) { this->pipe = pipe; }
	bool start() {
		bStarted = true;
		switch (type){
		case WRITE: {
			BOOL bFin = WriteFile(pipe, begin, end - begin, &length, &overlap);
			DWORD lastError = GetLastError();
			if (!bFin) {
				if (lastError == ERROR_IO_PENDING) fPending = true;
				else {
					showError("Failed to start Write Action", lastError);
					bFailed = true;
				}
			}
		} break;
		case READ: {
			BOOL bFin = ReadFile(pipe, begin, end - begin, &length, &overlap);
			DWORD lastError = GetLastError();
			if (!bFin) {
				if (lastError == ERROR_IO_PENDING) fPending = true;
				else {
					showError("Failed startet Write action!", lastError);
					bFailed = true;
				}
			}
		} break;
		}
		return !(fPending || bFailed);
	}
	virtual STATUS_CODE getState(BOOL wait = FALSE) = 0;
	const TASK_TYPE type;
protected:
	OVERLAPPED overlap;
	DWORD length;
	HANDLE pipe;
	T begin, end;
	bool fPending;
	bool bStarted;
	bool bFailed;
	void update(BOOL wait = FALSE) {
		BOOL bFin = GetOverlappedResult(pipe, &overlap, &length, wait);
		DWORD lastError = GetLastError();
		if (!bFin) {
			if (lastError == ERROR_IO_PENDING) fPending = true;
			else {
				showError("Failed Update", lastError);
				bFailed = true;
			}
		} else {
			fPending = false;
		}
	}
};

template <typename T, typename VEC = std::vector<char>>
class WriteTask : public Task<T> {
public:
	WriteTask(T begin, T end) : Task{ Task::TASK_TYPE::WRITE, begin, end } {}
	WriteTask(VEC&& data) : Task(Task::TASK_TYPE::WRITE, begin, end), _data{data} {}
	Task::STATUS_CODE getState(BOOL wait = FALSE) final {
		update(wait);
		if (bFailed) return Task::STATUS_CODE::FAILED;
		else if (!bStarted) return Task::STATUS_CODE::NOT_STARTED;
		else if (fPending) return Task::STATUS_CODE::PENDING;
		else return end - begin == length ? 
			Task::STATUS_CODE::SUCCESS
			: Task::STATUS_CODE::ERROR_TO_SHORT;
	}
	std::size_t writtenBytes() { return length; };
private:
	VEC _data;
};

template <typename T>
class ReadTask : public Task<T> {
public:
	ReadTask(T buffer, std::size_t length) : Task(Task::TASK_TYPE::READ, begin, end) {}
	Task::STATUS_CODE getState(BOOL wait = FALSE) final {
		update(wait);
		if (bFailed) return Task::STATUS_CODE::FAILED;
		else if (!bStarted) return Task::STATUS_CODE::NOT_STARTED;
		else if (fPending) return Task::STATUS_CODE::PENDING;
		else return Task::STATUS_CODE::SUCCESS;
	}
	std::size_t rededBytes() { return length; }
};

class PipeServer
{
public:
	enum PIPE {PIPE_IN, PIPE_OUT};
	PipeServer();
	~PipeServer();
	/** \return pipeId \param si startInformation for Child Process */
	std::size_t addConnection();
	/** \return exitcode: 0:success, 1:already closed, -1:error*/
	int closeConnection(std::size_t id);
	bool checkConnection(std::size_t id);
	void printPipe(std::size_t id);
	HANDLE duplicateHandler(PIPE dir, std::size_t id, HANDLE hTargetProcess);
	template <typename T>
	bool addTask(std::size_t id, Task<T>& task) { return _pipes[id].addTask<T>(task); }
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
	class DualChanelPipe {
	public:
		DualChanelPipe(DWORD bufferSize = 0)
			: readP{ bufferSize },
			writeP{ bufferSize },
			fConnected{ false }
		{
			fBroken = readP.fBroken || writeP.fBroken;
			fConnected = !fBroken;
		}
		void close();
		template <typename T>
		bool addTask(Task<T>& task) {
			if (task.type == Task<T>::TASK_TYPE::READ)
				task.setPipeHandle(readP.hRead);
			else if (task.type == Task<T>::TASK_TYPE::WRITE)
				task.setPipeHandle(writeP.hWrite);
			task.start();
			return true;
		}
		Pipe readP, writeP;
		BOOL fConnected, fBroken;
	};
	std::vector<DualChanelPipe> _pipes;
};
