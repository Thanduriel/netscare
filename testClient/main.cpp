#include <Windows.h>
#include <wininet.h>
#pragma comment( lib, "wininet" )
#include <tchar.h>
#include<iostream>

#include "../server/Asn.hpp"

int main() {
	HINTERNET hIntSession = InternetOpenA(_T("Client"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET hHttpSession = InternetConnectA(hIntSession, _T("localhost"), 9090, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	HINTERNET hHttpRequest = HttpOpenRequestA(hHttpSession, _T("POST"), _T("test1/"), NULL, "application/asn", NULL, INTERNET_FLAG_RELOAD, 0);
	TCHAR* szHeader = _T("Content-Type: text/plain");
	CHAR szReq[] = {0x13, 0x05, 'c', 'a', 'l', 'c', 0, 0x02, 0x01, 8, 0x02, 0x01, 5};
	ASNObject *asn = ASNObject::DecodeAsn(reinterpret_cast<unsigned char*>(szReq), 12);
	for (unsigned long pos = 0; pos < 13; pos += asn->GetSize(), ++asn) {
		std::cout << *asn;
	}
	if (!HttpSendRequestA(hHttpRequest, szHeader, _tcslen(szHeader), szReq, 13)) {
		printf("Error when send: %ul", GetLastError());
	}

	CHAR szBuffer[1024];
	DWORD dwRead = 0;
	while (InternetReadFile(hHttpRequest, szBuffer, sizeof(szBuffer) - 1, &dwRead) && dwRead) {
		szBuffer[dwRead] = 0;
		ASNObject* asnO =  ASNObject::DecodeAsn(reinterpret_cast<unsigned char*>(szBuffer), dwRead);
		std::cout << "Readed: " << asnO[0].DecodeString() << " = " << asnO[1].DecodeInteger();
		dwRead = 0;
	}

	InternetCloseHandle(hHttpRequest);
	InternetCloseHandle(hHttpSession);
	InternetCloseHandle(hIntSession);
}