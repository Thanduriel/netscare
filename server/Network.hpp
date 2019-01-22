#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define MAX_ULONG_STR ((ULONG) sizeof("4294967295"))

#include <windows.h>
#include <http.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

#pragma comment(lib, "httpapi.lib")

#include "Asn.hpp"
#include "Types.hpp"

class NetScareServer {
public:
	struct Action {
		enum TYPE { NOT_SET, USER_NEW, PIC_LOAD, EVENT_ADD, TRIGGER_EVENT } type;
		Action() : type{ NOT_SET }, data{ nullptr } {}
		char *data;
	};
private:
	int urlC;
	wchar_t **urls;
	int urlAdded;
	HANDLE hReqQueue;
	ULONG redCode;
	OVERLAPPED overlapped;
	bool bPending;

	HTTP_REQUEST_ID    requestId;
	DWORD              bytesRead;
	PHTTP_REQUEST      pRequest;
	PCHAR              pRequestBuffer;
	ULONG RequestBufferLength = sizeof(HTTP_REQUEST) + 2048; // buffer size = 2KB
	DWORD SendHttpResponse(
		IN HANDLE        hReqQueue,
		IN PHTTP_REQUEST pRequest,
		IN USHORT        StatusCode,
		IN PSTR          pReason,
		IN PSTR          pEntity
	);
	DWORD SendHttpPostResponse(
		Action &action,
		HANDLE        hReqQueue,
		PHTTP_REQUEST pRequest
	);
	std::vector<User>& users;
public:
	NetScareServer(int urlC, wchar_t **urls, std::vector<User>& users);
	~NetScareServer();
	ULONG GetLastRedCode() { return redCode; }
	ULONG updateServer(Action& action, BOOL wait = FALSE);
};

void INITIALIZE_HTTP_RESPONSE(HTTP_RESPONSE* resp, USHORT status, const char* reason);

void ADD_KNOWN_HEADER(HTTP_RESPONSE& Response, int HeaderId, const char* RawValue);

void* ALLOC_MEM(std::size_t size);

void FREE_MEM(void* ptr);
