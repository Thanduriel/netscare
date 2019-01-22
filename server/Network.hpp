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
#include <stdio.h>
#include <cstdlib>

#pragma comment(lib, "httpapi.lib")

#define INITIALIZE_HTTP_RESPONSE( resp, status, reason )    \
    do                                                      \
    {                                                       \
        RtlZeroMemory( (resp), sizeof(*(resp)) );           \
        (resp)->StatusCode = (status);                      \
        (resp)->pReason = (reason);                         \
        (resp)->ReasonLength = (USHORT) strlen(reason);     \
    } while (FALSE)

#define ADD_KNOWN_HEADER(Response, HeaderId, RawValue)               \
    do                                                               \
    {                                                                \
        (Response).Headers.KnownHeaders[(HeaderId)].pRawValue =      \
                                                          (RawValue);\
        (Response).Headers.KnownHeaders[(HeaderId)].RawValueLength = \
            (USHORT) strlen(RawValue);                               \
    } while(FALSE)

#define ALLOC_MEM(cb) HeapAlloc(GetProcessHeap(), 0, (cb))

#define FREE_MEM(ptr) HeapFree(GetProcessHeap(), 0, (ptr))

//
// Prototypes.
//

class NetScareServer {
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
public:
	struct Action {
		enum TYPE { NOT_SET, USER_NEW, PIC_LOAD, EVENT_ADD, TRIGGER_EVENT } type;
		Action() : type{ NOT_SET }, data{ nullptr } {}
		char *data;
	};
	NetScareServer(int urlC, wchar_t **urls);
	~NetScareServer();
	ULONG GetLastRedCode() { return redCode; }
	ULONG updateServer(Action& action, BOOL wait = FALSE);
};

DWORD
SendHttpResponse(
	IN HANDLE        hReqQueue,
	IN PHTTP_REQUEST pRequest,
	IN USHORT        StatusCode,
	IN PSTR          pReason,
	IN PSTR          pEntity
);

DWORD
SendHttpPostResponse(
	NetScareServer::Action &action,
	IN HANDLE        hReqQueue,
	IN PHTTP_REQUEST pRequest
);

