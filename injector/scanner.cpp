#include "scanner.hpp"
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <fstream>

ProcessScanner::ProcessScanner()
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		
	}


	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.

	for (i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != 0)
		{
			ScanProcess(aProcesses[i]);
		}
	}
}


void ProcessScanner::ScanProcess(DWORD _processID)
{
	static std::ofstream logFile("foo.txt");
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	// Get a handle to the process.

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, _processID);

	// Get the process name.

	if (NULL != hProcess)
	{
		HMODULE hMods[1024];
		DWORD cbNeeded;

		if (EnumProcessModules(hProcess, hMods, sizeof(hMods),
			&cbNeeded))
		{
			GetModuleBaseName(hProcess, hMods[0], szProcessName,
				sizeof(szProcessName) / sizeof(TCHAR));

			for (size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				TCHAR szModName[MAX_PATH];

				// Get the full path to the module's file.

				if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
					sizeof(szModName) / sizeof(TCHAR))
					&& strstr(szModName, "d3d11.dll"))
				{
					logFile << szModName << std::endl;
					m_targetProcesses.push_back({ szProcessName , _processID });
					break;
				}
			}
		}
	}

	logFile << szProcessName << " PID: " << _processID << "\n" << std::endl;

	CloseHandle(hProcess);
}

