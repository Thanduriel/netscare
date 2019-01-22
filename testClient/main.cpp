#include <Windows.h>
#include <wininet.h>
#pragma comment( lib, "wininet" )
#include <tchar.h>
#include<iostream>

int main() {
	HINTERNET hIntSession = InternetOpenA(_T("Client"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET hHttpSession = InternetConnectA(hIntSession, _T("localhost"), 9090, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	HINTERNET hHttpRequest = HttpOpenRequestA(hHttpSession, _T("POST"), _T("test1/"), NULL, "application/asn", NULL, INTERNET_FLAG_RELOAD, 0);
	TCHAR* szHeader = _T("Content-Type: text/plain");
	CHAR szReq[] = "x=5&y=8";
	if (!HttpSendRequestA(hHttpRequest, szHeader, _tcslen(szHeader), szReq, strlen(szReq))) {
		printf("Error when send: %ul", GetLastError());
	}

	CHAR szBuffer[1024];
	DWORD dwRead = 0;
	while (InternetReadFile(hHttpRequest, szBuffer, sizeof(szBuffer) - 1, &dwRead) && dwRead) {
		szBuffer[dwRead] = 0;
		printf("GetFile:\t%s", szBuffer);
		dwRead = 0;
	}

	InternetCloseHandle(hHttpRequest);
	InternetCloseHandle(hHttpSession);
	InternetCloseHandle(hIntSession);
}