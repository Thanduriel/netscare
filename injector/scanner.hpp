#pragma once

#include <Windows.h>
#include <vector>
#include <string>

struct ProcessInfo
{
	std::string name;
	DWORD ID;
};

class ProcessScanner
{
public:
	ProcessScanner();

	auto begin() { return m_targetProcesses.begin(); }
	auto end() { return m_targetProcesses.end(); }
private:
	void ScanProcess(DWORD _processID);

	std::vector<ProcessInfo> m_targetProcesses;
};