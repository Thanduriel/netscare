#include <windows.h>
#include <stdio.h>
#include <detours.h>
#include <d3d.h>

extern "C" DWORD WINAPI Hidden(DWORD dwCount)
{
	printf("target.dll:     Hidden(%d) -> %d.\n", dwCount, dwCount + 1);
	return dwCount + 1;
}

// We use this point to ensure Hidden isn't inlined.
static DWORD(WINAPI * SelfHidden)(DWORD dwCount) = Hidden;

DWORD WINAPI Target(DWORD dwCount)
{
	printf("target.dll:   Target  (%d) -> %d.\n", dwCount, dwCount + 100);
	dwCount = SelfHidden(dwCount + 100);
	printf("target.dll:   Target  (.....) -> %d.\n", dwCount);
	return dwCount;
}

int __cdecl main(void)
{
	printf("findfunc.exe: Starting.\n");
	fflush(stdout);

	printf("DLLs:\n");
	for (HMODULE hModule = NULL; (hModule = DetourEnumerateModules(hModule)) != NULL;) {
		CHAR szName[MAX_PATH] = { 0 };
		GetModuleFileNameA(hModule, szName, sizeof(szName) - 1);
		printf("  %p: %s\n", hModule, szName);
	}

	DWORD dwCount = 10000;
	for (int i = 0; i < 3; i++) {
		printf("findfunc.exe: Calling (%d).\n", dwCount);
		dwCount = Target(dwCount) + 10000;
	}
	return 0;
}