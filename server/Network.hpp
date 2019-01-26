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
#include <fstream>
#include <filesystem>

#pragma comment(lib, "httpapi.lib")

#include "Asn.hpp"
#include "Types.hpp"
#include "Messages.hpp"

class NetScareServer {
public:
	struct Action {
		enum TYPE { NOT_SET, USER_NEW, PIC_LOAD, EVENT_ADD, TRIGGER_EVENT } type;
		Action() : type{ NOT_SET }, data{ nullptr } {}
		char *data;
	};
private:
	enum RESPONDERRORS { SUCCEESS,  INVALID_DATA, INTERNAL_ERROR };
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
	std::vector<User>& _users;
	Tickets& _tickets;
	struct CommandHis {
		std::unique_ptr<Command> command;
		std::uint8_t target;
		std::uint8_t src;
		CommandHis(std::unique_ptr<Command>&& cmd, std::uint8_t targte, std::uint8_t src) : command{ std::move(cmd) }, target{ targte }, src{ src } {}
	};
	std::vector<CommandHis> _commandHistory;
	RESPONDERRORS CalculationRespond(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len);
	RESPONDERRORS LoginResponde(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len);
	RESPONDERRORS LoadPictureResponde(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len);
	RESPONDERRORS UpdateStateResponde(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len);
public:
	NetScareServer(int urlC, wchar_t **urls, std::vector<User>& users, Tickets& tickets);
	~NetScareServer();
	ULONG GetLastRedCode() { return redCode; }
	ULONG UpdateServer(OUT Action& action, BOOL wait = FALSE);
};

void INITIALIZE_HTTP_RESPONSE(HTTP_RESPONSE* resp, USHORT status, const char* reason);

void ADD_KNOWN_HEADER(HTTP_RESPONSE& Response, int HeaderId, const char* RawValue);

void* ALLOC_MEM(std::size_t size);

void FREE_MEM(void* ptr);

constexpr static unsigned long myStrLen(const char* str) {
	unsigned long l = 0;
	while (*(++str) != 0) ++l;
	return l;
}


constexpr char* MSG_USEREXIST = "Username already taken!";
constexpr char* MSG_NOTIMPLEMENTED = "Function ont Avalible now!";
constexpr char* MSG_INVALIDDATA = "Requets is not well fromed!";
constexpr char* MSG_INTERNELERROR = "Server Error, sporry";
constexpr char* MSG_UNKNOWNUSER = "UserId is not Registerd!";
constexpr char* MSG_UNNOKNCOMMAND = "Cannot interpret Command!";
// constexpr const unsigned char *ASN_FAILED = ASNObject::EncodeAsnPrimitives("failed");
