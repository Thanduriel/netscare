#include <Windows.h>
#include <wininet.h>
#pragma comment( lib, "wininet" )
#include <tchar.h>
#include<iostream>

#include "../server/Asn.hpp"

int __cdecl wmain(int argc, wchar_t **argv) {
	HINTERNET hIntSession = InternetOpenA(_T("Client"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	HINTERNET hHttpSession = InternetConnectA(hIntSession, _T("localhost"), 9090, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	HINTERNET hHttpRequest = HttpOpenRequestA(hHttpSession, _T("POST"), _T("test1/"), NULL, "application/asn", NULL, INTERNET_FLAG_RELOAD, 0);
	TCHAR* szHeader = _T("Content-Type: text/plain");


	const wchar_t* username = argc > 1 ? argv[1] : L"Zabuza";
	unsigned long decode[] = {
		ASNObject::EncodingSize("login"),
		ASNObject::EncodingSize(username),
	};
	unsigned long len = decode[0] + decode[1];
	CHAR *szReq = new CHAR[len];
	ASNObject::EncodeAsnPrimitives("login", reinterpret_cast<unsigned char*>(szReq));
	ASNObject::EncodeAsnPrimitives(username, reinterpret_cast<unsigned char*>(szReq) + decode[0]);
	ASNObject *asn = ASNObject::DecodeAsn(reinterpret_cast<unsigned char*>(szReq), len);
	for (unsigned long pos = 0; pos < len; pos += asn->GetSize(), ++asn) {
		if (asn->GetType() != ASNObject::UTF8String) std::cout << *asn;
		else std::wcout << L"UTF8: " << asn->DecodeUTF8() << L'\n';
	}
	if (!HttpSendRequestA(hHttpRequest, szHeader, _tcslen(szHeader), szReq, len)) {
		printf("Error when send: %ul", GetLastError());
	}
	
	delete szReq;

	CHAR szBuffer[1024];
	DWORD dwRead = 0;
	while (InternetReadFile(hHttpRequest, szBuffer, sizeof(szBuffer) - 1, &dwRead) && dwRead) {
		szBuffer[dwRead] = 0;
		ASNObject* asnO =  ASNObject::DecodeAsn(reinterpret_cast<unsigned char*>(szBuffer), dwRead);
		for (unsigned long pos = 0; pos < dwRead; pos += asnO->GetSize(), ++asnO) {
			std::cout << *asnO;
		}
		dwRead = 0;
	}

	InternetCloseHandle(hHttpRequest);
	InternetCloseHandle(hHttpSession);
	InternetCloseHandle(hIntSession);
}