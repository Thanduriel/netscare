#include <io.h>

#include "Network.hpp"

namespace fs = std::filesystem;

NetScareServer::NetScareServer(int urlC, wchar_t **urls, std::vector<User>& users, Tickets& tickets) : urlC{ urlC }, urls{ urls }, hReqQueue{ NULL }, urlAdded{ 0 }, bPending{ false }, overlapped{ 0 }, _users{ users }, _tickets{ tickets} {
	ULONG retCode;
	HTTPAPI_VERSION httpApiVersion = HTTPAPI_VERSION_1;
	DWORD a = 0;
	retCode = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_SERVER, NULL);
	if (retCode != NO_ERROR) {
		wprintf(L"HttpInit failed with: %lu\n", retCode);
		return;
	}
	a = GetLastError();

	retCode = HttpCreateHttpHandle(&hReqQueue, 0);
	if (retCode != NO_ERROR) {
		wprintf(L"HttpCreaetHadnle failed with: %ul\n", retCode);
		return;
	}
	a = GetLastError();

	for (int i = 1; i < urlC; ++i) {
		wprintf(L"listening for requests on the following url: %s\n", urls[i]);
		retCode = HttpAddUrl(hReqQueue, urls[i], NULL);
		if (retCode != NO_ERROR) {
			wprintf(L"HttpAddUrl failed with %lu \n", retCode);
			// this->~NetScareServer();
		}
		else {
			++urlAdded;
		}
		a = GetLastError();
	}
	pRequestBuffer = reinterpret_cast<PCHAR>(ALLOC_MEM(RequestBufferLength));
	retCode = ERROR_NOT_ENOUGH_MEMORY;
	if (pRequestBuffer == NULL) return;
	pRequest = reinterpret_cast<PHTTP_REQUEST>(pRequestBuffer);
	a = GetLastError();
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	a = GetLastError();
}

NetScareServer::~NetScareServer() {
	FREE_MEM(pRequestBuffer);
	for (int i = 1; i <= urlAdded; ++i) {
		HttpRemoveUrl(hReqQueue, urls[i]);
	}
	if (hReqQueue) CloseHandle(hReqQueue);
	HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
	HTTP_SET_NULL_ID(&requestId);
}

DWORD NetScareServer::UpdateServer(Action& action, BOOL wait) {
	ULONG              result;
	DWORD a = 0;
	action.type = Action::NOT_SET;
	if (!bPending) {
		ResetEvent(overlapped.hEvent);
		HANDLE hEv = overlapped.hEvent;
		overlapped = { 0 };
		overlapped.hEvent = hEv;
		RtlZeroMemory(pRequest, RequestBufferLength);
		a = GetLastError();
		result = HttpReceiveHttpRequest(
			hReqQueue,
			requestId,
			0,
			pRequest,
			RequestBufferLength,
			NULL,
			&overlapped
		);
		a = GetLastError();
	} else {
		if (overlapped.Internal == STATUS_PENDING) return ERROR_IO_PENDING;
		wprintf_s(L"Fin");
		a = GetLastError();
		a = GetLastError();
		if (!GetOverlappedResult(hReqQueue, &overlapped, &bytesRead, TRUE)) {
			wprintf_s(L"GetOverError %ul", GetLastError());
			result = GetLastError();
		} else {
			result = 0;
		}
		bPending = false;
	}

	if (result == ERROR_IO_PENDING) {
		a = GetLastError();
		bPending = true;
		return ERROR_IO_PENDING;
	} else if (result == NO_ERROR) {
		switch (pRequest->Verb)
		{
		case HttpVerbGET:
			wprintf(L"Got a GET request for %ws \n",
				pRequest->CookedUrl.pFullUrl);
			result = SendHttpResponse(
				hReqQueue,
				pRequest,
				200,
				"OK",
				"Hey! You hit the server \r\n"
			);
			break;
		case HttpVerbPOST:
			wprintf(L"Got a POST request for %ws \n",
				pRequest->CookedUrl.pFullUrl);
			result = SendHttpPostResponse(action, hReqQueue, pRequest);
			break;
		default:
			wprintf(L"Got a unknown request for %ws \n",
				pRequest->CookedUrl.pFullUrl);
			result = SendHttpResponse(
				hReqQueue,
				pRequest,
				503,
				"Not Implemented",
				NULL
			);
		break;
		}
		
		if (result != NO_ERROR) { return result;}
		HTTP_SET_NULL_ID(&requestId);
	}
	else if (result == ERROR_MORE_DATA)
	{
		requestId = pRequest->RequestId;
		RequestBufferLength = bytesRead;
		FREE_MEM(pRequestBuffer);
		pRequestBuffer = reinterpret_cast<PCHAR>(ALLOC_MEM(RequestBufferLength));
		
		if (pRequestBuffer == NULL) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		pRequest = (PHTTP_REQUEST)pRequestBuffer;

	}
	else if (ERROR_CONNECTION_INVALID == result && !HTTP_IS_NULL_ID(&requestId))
	{
		HTTP_SET_NULL_ID(&requestId);
	}
	else
	{
		wprintf_s(L"Random error %ul", result);
		return result;
	}
	return 0;
}

DWORD NetScareServer::SendHttpResponse(
	IN HANDLE        hReqQueue,
	IN PHTTP_REQUEST pRequest,
	IN USHORT        StatusCode,
	IN PSTR          pReason,
	IN PSTR          pEntityString
)
{
	HTTP_RESPONSE   response;
	HTTP_DATA_CHUNK dataChunk;
	DWORD           result;
	DWORD           bytesSent;

	//
	// Initialize the HTTP response structure.
	//
	INITIALIZE_HTTP_RESPONSE(&response, StatusCode, pReason);

	//
	// Add a known header.
	//
	ADD_KNOWN_HEADER(response, HttpHeaderContentType, "text/html");

	if (pEntityString)
	{
		// 
		// Add an entity chunk.
		//
		dataChunk.DataChunkType = HttpDataChunkFromMemory;
		dataChunk.FromMemory.pBuffer = pEntityString;
		dataChunk.FromMemory.BufferLength =
			(ULONG)strlen(pEntityString);

		response.EntityChunkCount = 1;
		response.pEntityChunks = &dataChunk;
	}

	// 
	// Because the entity body is sent in one call, it is not
	// required to specify the Content-Length.
	//

	result = HttpSendHttpResponse(
		hReqQueue,           // ReqQueueHandle
		pRequest->RequestId, // Request ID
		0,                   // Flags
		&response,           // HTTP response
		NULL,                // pReserved1
		&bytesSent,          // bytes sent  (OPTIONAL)
		NULL,                // pReserved2  (must be NULL)
		0,                   // Reserved3   (must be 0)
		NULL,                // LPOVERLAPPED(OPTIONAL)
		NULL                 // pReserved4  (must be NULL)
	);

	if (result != NO_ERROR)
	{
		wprintf(L"HttpSendHttpResponse failed with %lu \n", result);
	}

	return result;
}

NetScareServer::RESPONDERRORS NetScareServer::CalculationRespond(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len) {
	if (asn.len != 3 
		|| asn.objects[1].GetType() != ASNObject::INTEGER
		|| asn.objects[2].GetType() != ASNObject::INTEGER) return RESPONDERRORS::INVALID_DATA;
	u_char x = static_cast<std::uint8_t>(asn.objects[1].DecodeInteger());
	u_char y = static_cast<std::uint8_t>(asn.objects[2].DecodeInteger());
	msg = new unsigned char[9]{ u_char(0x13), u_char(0x04), 'r', 'e', 's', u_char(0), u_char(0x02), u_char(0x01), u_char(x + y) };
	len = 9;
	return RESPONDERRORS::SUCCEESS;
}

NetScareServer::RESPONDERRORS NetScareServer::LoginResponde(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len) {
	if (asn.len != 2
		|| asn.objects[1].GetType() != ASNObject::UTF8String) return RESPONDERRORS::INVALID_DATA;
	const wchar_t* name = asn.objects[1].DecodeUTF8();
	bool doublicate = false;
	for (const User& u : _users) {
		if (wcscmp(asn.objects[1].DecodeUTF8(), u.GetName().c_str()) == 0) {
			doublicate = true;
			break;
		}
	}
	if (doublicate) {
		unsigned long decode[] = {
			ASNObject::EncodingSize(NM_FAILED),
			ASNObject::EncodingSize(MSG_USEREXIST)
		};
		len = decode[0] + decode[1];
		msg = new unsigned char[len];
		ASNObject::EncodeAsnPrimitives(NM_FAILED, msg);
		ASNObject::EncodeAsnPrimitives(MSG_USEREXIST, msg + decode[0]);
	}
	else {
		_users.push_back(User(asn.objects[1].DecodeUTF8()));
		_tickets.AddUser();
		std::wcout << L"AddedUser: " << _users.back().GetName().c_str() << '\n';
		unsigned long decode[] = {
			ASNObject::EncodingSize(NM_SUCCESS),
			ASNObject::EncodingSize(_users.back().id)
		};
		len = decode[0] + decode[1];
		msg = new unsigned char[len];
		ASNObject::EncodeAsnPrimitives(NM_SUCCESS, msg);
		ASNObject::EncodeAsnPrimitives(_users.back().id, msg + decode[0]);
		_commandHistory.emplace_back(std::make_unique<AddUserCommand>(_users.back().GetName().c_str(), _users.back().id), 0xFF, _users.back().id);
	}
	return RESPONDERRORS::SUCCEESS;
}

NetScareServer::RESPONDERRORS NetScareServer::UpdateStateResponde(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len) {
	if (asn.len > 3
		|| asn.len < 2
		|| asn.objects[1].GetType() != ASNObject::INTEGER
		|| asn.len == 3 && asn.objects[2].GetType() != ASNObject::SEQUENCE) {
		return RESPONDERRORS::INVALID_DATA;
	}
	std::cout << "check\n";
	int uId = asn.objects[1].DecodeInteger();
	// check if user Exist
	bool findUser = false;
	if (uId >= 0) {
		for (const User& u : _users) {
			if (u.id == uId) {
				findUser = true;
				break;
			}
		}
	}
	if (findUser) {
		if (asn.len >= 3) {
			ASNObject::ASNDecodeReturn commands = asn.objects[2].DecodeASNObjscets();
			for (unsigned long i = 0; i < commands.len; ++i) {
				if (commands.objects[i].GetType() != ASNObject::SEQUENCE) return RESPONDERRORS::INVALID_DATA;
				std::unique_ptr<Command> cmd = Command::Decode(commands.objects[i].DecodeASNObjscets());
				if (cmd->GetState() == Command::DECODERESULT::CORRUPTED) return RESPONDERRORS::INVALID_DATA;
				switch (cmd->GetType()) {
				case Command::TYPE::TRIGGEREVENT: {
					TriggerEventCommand * tcd = dynamic_cast<TriggerEventCommand*>(cmd.get());
					if (!tcd) std::cout << "Failed to Tecode TriggerEvent!\n";
					else {
						int eId = tcd->GetEventId();
						if (static_cast<unsigned int>(uId) >= _users.size()) std::cout << "UnknownUser Id " << static_cast<int>(uId) << "\n";
						if (_tickets.GetTicket(uId, _users[uId].GetEvent(eId).target) <= 0) {
							std::cout << "Trigger Event no Tickets: " << eId << " from User: " << static_cast<int>(uId) << '\n';
							continue;
						}
						_commandHistory.emplace_back(std::move(cmd), _users[uId].GetEvent(eId).target, uId);
						_commandHistory
						std::cout << "Trigger EventId " << eId << " from userId: " << static_cast<int>(uId) << '\n';
					}
				} break;
				case Command::TYPE::ADDEVENT: {
					AddEventCommand *aec = dynamic_cast<AddEventCommand*>(cmd.get());
					if (!aec) std::cout << "Failed to Decode AddEvent!\n";
					else {
						AddEventCommand::Event ev = aec->GetEvent();
						_commandHistory.emplace_back(std::move(cmd), ev.target, uId);
						char fileName[32];
						sprintf_s(fileName, "%d_%d.dds", uId, ev.pId);
						bool ac = _access(fileName, 0) == 0;
						_users[uId].AddEvent(ev.target, ev.pId, uId, ac);
						if (ac) {
							_commandHistory.emplace_back(std::make_unique<LoadPictureCommand>(fileName), ev.target, uId);
							std::cout << "Chained File\n";
						}
						else {
							std::cout << "NoFile: " << fileName << '\n';
						}
						std::cout << "AddEvent: " << ev.eventId << " pId:" << static_cast<int>(ev.pId) << " target: " << static_cast<int>(ev.target) << '\n';
					}
				}	break;
				default: {
					unsigned long decode[] = {
						ASNObject::EncodingSize(NM_FAILED),
						ASNObject::EncodingSize(MSG_UNNOKNCOMMAND)
					};
					len = decode[0] + decode[1];
					msg = new unsigned char[len];
					ASNObject::EncodeAsnPrimitives(NM_FAILED, msg);
					ASNObject::EncodeAsnPrimitives(MSG_UNNOKNCOMMAND, msg + decode[0]);
					return RESPONDERRORS::SUCCEESS;
				}
				}
			}
		}
	} else {
		unsigned long decode[] = {
			ASNObject::EncodingSize(NM_FAILED),
			ASNObject::EncodingSize(MSG_UNKNOWNUSER)
		};
		len = decode[0] + decode[1];
		msg = new unsigned char[len];
		ASNObject::EncodeAsnPrimitives(NM_FAILED, msg);
		ASNObject::EncodeAsnPrimitives(MSG_UNKNOWNUSER, msg + decode[0]);
	}

	// createRespond Mesage
	len = 0;
	unsigned long offset[] = {
		ASNObject::EncodingSize(NM_SUCCESS),
	};
	for (unsigned long l : offset) len += l;

	std::unique_ptr<UpdateTicketsCommand>update = nullptr;

	std::vector<const Command*> cmds(0);
	if (_users[uId].executedCommands < _commandHistory.size()) {
		for (auto itr = _commandHistory.begin() + _users[uId].executedCommands; itr < _commandHistory.end(); ++itr) {
			if (itr->target == 0xFF || itr->target == uId) {
				cmds.emplace_back(itr->command.get());
			}
		}
		update = std::make_unique<UpdateTicketsCommand>(_tickets, uId);
		cmds.emplace_back(update.get());
		_users[uId].executedCommands = _commandHistory.size();
		len += ASNObject::EncodingSize(cmds);
		std::cout << "update " << cmds.size() << '\n';
	}

	msg = new unsigned char[len];

	ASNObject::EncodeAsnPrimitives(NM_SUCCESS, msg);
	if(cmds.size())	ASNObject::EncodeSequence(cmds, msg + offset[0]);

	return RESPONDERRORS::SUCCEESS;
}


NetScareServer::RESPONDERRORS NetScareServer::LoadPictureResponde(const ASNObject::ASNDecodeReturn& asn, OUT unsigned char*& msg, OUT unsigned long& len) {
	if (asn.len != 4
		|| asn.objects[1].GetType() != ASNObject::INTEGER
		|| asn.objects[2].GetType() != ASNObject::INTEGER
		|| asn.objects[3].GetType() != ASNObject::OCTASTRING) return RESPONDERRORS::INVALID_DATA;
	int userId = asn.objects[1].DecodeInteger();
	int picId = asn.objects[2].DecodeInteger();
	std::string filename(std::to_string(userId) + '_' + std::to_string(picId) + ".dds");
	std::cout << "Start load Pictzure: uID: " << userId << "  picId: " << picId << "  filename: " << filename << '\n';
	std::ofstream file(filename, std::ios::binary);
	if (file.fail()) { std::cout << "cant open file\n"; return RESPONDERRORS::INTERNAL_ERROR; }
	file.write(reinterpret_cast<const char*>(asn.objects[3].GetRaw()), asn.objects[3].GetSize());
	if (file.fail()) { std::cout << "cant write file\n"; return RESPONDERRORS::INTERNAL_ERROR; }
	file.close();

	unsigned long decode[] = {
		ASNObject::EncodingSize(NM_SUCCESS)
	};
	len = decode[0];
	msg = new unsigned char[len];
	ASNObject::EncodeAsnPrimitives(NM_SUCCESS, msg);

	return RESPONDERRORS::SUCCEESS;
}

DWORD NetScareServer::SendHttpPostResponse(
	Action &action,
	HANDLE        hReqQueue,
	PHTTP_REQUEST pRequest
)
{
	HTTP_RESPONSE   response;
	DWORD           result;
	DWORD           bytesSent;
	PUCHAR          pEntityBuffer;
	ULONG           EntityBufferLength;
	ULONG           BytesRead;
	CHAR            szContentLength[MAX_ULONG_STR];
	HTTP_DATA_CHUNK dataChunk;
	ULONG           TotalBytesRead = 0;

	BytesRead = 0;

	std::cout << "Start Read POts Requets\n";

	EntityBufferLength = 1024 * 2048;
	pEntityBuffer = reinterpret_cast<PUCHAR>(ALLOC_MEM(EntityBufferLength));

	if (pEntityBuffer == NULL)
	{
		result = ERROR_NOT_ENOUGH_MEMORY;
		wprintf(L"Insufficient resources \n");
		goto Done;
	}

	//
	// Initialize the HTTP response structure.
	//
	INITIALIZE_HTTP_RESPONSE(&response, 200, "OK");
	if (pRequest->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS)
	{
		std::vector<unsigned char> input;
		do
		{
			//
			// Read the entity chunk from the request.
			//
			BytesRead = 0;
			result = HttpReceiveRequestEntityBody(
				hReqQueue,
				pRequest->RequestId,
				0,
				pEntityBuffer,
				EntityBufferLength,
				&BytesRead,
				NULL
			);

			switch (result)
			{
			case NO_ERROR:

				if (BytesRead != 0)
				{
					TotalBytesRead += BytesRead;
					input.insert(input.end(), pEntityBuffer, pEntityBuffer + BytesRead);
				}
				break;

			case ERROR_HANDLE_EOF: {
				if (BytesRead != 0)
				{
					TotalBytesRead += BytesRead;
					input.insert(input.end(), pEntityBuffer, pEntityBuffer + BytesRead);
				}
				ASNObject::ASNDecodeReturn asn = ASNObject::DecodeAsn(input.data(), TotalBytesRead);
				
				unsigned char *msg = nullptr;
				unsigned long len = 0;

				RESPONDERRORS resultState = RESPONDERRORS::INVALID_DATA;
				if (asn.objects[0].GetType() == ASNObject::ASCISTRING) {
					if (strcmp(asn[0].DecodeString(), "calc") == 0) {
						resultState = CalculationRespond(asn, msg, len);
					} else if (strcmp(asn[0].DecodeString(), NM_LOGIN) == 0) {
						resultState = LoginResponde(asn, msg, len);
					} else if (strcmp(asn[0].DecodeString(), NM_LOADPIC) == 0) {
						resultState = LoadPictureResponde(asn, msg, len);
					} else if (strcmp(asn[0].DecodeString(), NM_UPDATE) == 0){
						resultState = UpdateStateResponde(asn, msg, len);
					} else std::cout << "No valid operation: " << asn.objects[0].DecodeString() << '\n';
				} else std::cout << "Cant resolve Requets No String Type: " << asn.objects[0].GetType() << '\n';

				if (resultState == RESPONDERRORS::INVALID_DATA || resultState == RESPONDERRORS::INTERNAL_ERROR) {
					if (resultState == RESPONDERRORS::INVALID_DATA) std::cout << "Invalid data\n";
					else std::cout << "Internel error\n";
					if (msg) delete[] msg;
					unsigned long decode[] = {
						ASNObject::EncodingSize(NM_FAILED),
						0
					};
					switch (resultState) {
					case RESPONDERRORS::INVALID_DATA: decode[1] = ASNObject::EncodingSize(MSG_INVALIDDATA); break;
					case RESPONDERRORS::INTERNAL_ERROR: decode[1] = ASNObject::EncodingSize(MSG_INTERNELERROR); break;
					}
					len = decode[0] + decode[1];
					msg = new unsigned char[len];
					ASNObject::EncodeAsnPrimitives(NM_FAILED, msg);
					switch (resultState) {
					case RESPONDERRORS::INVALID_DATA: ASNObject::EncodeAsnPrimitives(MSG_INVALIDDATA, msg + decode[0]); break;
					case RESPONDERRORS::INTERNAL_ERROR: ASNObject::EncodeAsnPrimitives(MSG_INTERNELERROR, msg + decode[0]); break;
					}
					
				}

				sprintf_s(szContentLength, MAX_ULONG_STR, "%lu", len);

				ADD_KNOWN_HEADER(
					response,
					HttpHeaderContentLength,
					szContentLength
				);

				result =
					HttpSendHttpResponse(
						hReqQueue,           // ReqQueueHandle
						pRequest->RequestId, // Request ID
						HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
						&response,       // HTTP response
						NULL,            // pReserved1
						&bytesSent,      // bytes sent-optional
						NULL,            // pReserved2
						0,               // Reserved3
						NULL,            // LPOVERLAPPED
						NULL             // pReserved4
					);

				if (result != NO_ERROR)
				{
					wprintf(
						L"HttpSendHttpResponse failed with %lu \n",
						result
					);
					goto Done;
				}
				/* dataChunk.DataChunkType = HttpDataChunkFromFileHandle;
				dataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
				dataChunk.FromFileHandle.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;
				dataChunk.FromFileHandle.FileHandle = hTempFile; */

				dataChunk.DataChunkType = HttpDataChunkFromMemory;
				dataChunk.FromMemory.pBuffer = msg;
				dataChunk.FromMemory.BufferLength = len;

				result = HttpSendResponseEntityBody(
					hReqQueue,
					pRequest->RequestId,
					0,           // This is the last send.
					1,           // Entity Chunk Count.
					&dataChunk,
					NULL,
					NULL,
					0,
					NULL,
					NULL
				);

				if (result != NO_ERROR)
				{
					wprintf(
						L"HttpSendResponseEntityBody failed %lu\n",
						result
					);
				}

				goto Done;

			}	break;


			default:
				wprintf(
					L"HttpReceiveRequestEntityBody failed with %lu \n",
					result);
				goto Done;
			}

		} while (TRUE);
	}
	else
	{
		result = HttpSendHttpResponse(
			hReqQueue,           // ReqQueueHandle
			pRequest->RequestId, // Request ID
			0,
			&response,           // HTTP response
			NULL,                // pReserved1
			&bytesSent,          // bytes sent (optional)
			NULL,                // pReserved2
			0,                   // Reserved3
			NULL,                // LPOVERLAPPED
			NULL                 // pReserved4
		);
		if (result != NO_ERROR)
		{
			wprintf(L"HttpSendHttpResponse failed with %lu \n",
				result);
		}
	}

Done:

	if (pEntityBuffer) FREE_MEM(pEntityBuffer);
	return result;
}

void INITIALIZE_HTTP_RESPONSE(HTTP_RESPONSE* resp, USHORT status, const char* reason) {
	RtlZeroMemory(resp, sizeof(HTTP_RESPONSE));
	resp->StatusCode = status;
	resp->pReason = reason;
	resp->ReasonLength = static_cast<USHORT>(strlen(reason));
}

void ADD_KNOWN_HEADER(HTTP_RESPONSE& Response, int HeaderId, const char* RawValue) {
	(Response).Headers.KnownHeaders[HeaderId].pRawValue = RawValue;
	(Response).Headers.KnownHeaders[HeaderId].RawValueLength = static_cast<USHORT>(strlen(RawValue));
}

void* ALLOC_MEM(std::size_t size) { return HeapAlloc(GetProcessHeap(), 0, size); }

void FREE_MEM(void* ptr) { HeapFree(GetProcessHeap(), 0, ptr); }

bool CmpAddress(PSOCKADDR addr1, PSOCKADDR addr2) {
	if (addr1->sa_family != addr2->sa_family) return false;
	if (addr1->sa_family == AF_INET6) {
		for (int i = 0; i < 8; ++i)
			if (reinterpret_cast<PSOCKADDR_IN6>(addr1)->sin6_addr.u.Word[i]
				!= reinterpret_cast<PSOCKADDR_IN6>(addr2)->sin6_addr.u.Word[i])
				return false;
		return true;
	}
	if (addr1->sa_family == AF_INET) {
		return reinterpret_cast<PSOCKADDR_IN>(addr1)->sin_addr.S_un.S_addr
			== reinterpret_cast<PSOCKADDR_IN>(addr2)->sin_addr.S_un.S_addr;
	}
	return false;
}

void PrintAddress(PSOCKADDR address) {
	if (address->sa_family == AF_INET6) {
		std::cout << "IPv6: " << std::hex;
		for (int i = 0; i < 8; ++i) {
			std::cout << reinterpret_cast<PSOCKADDR_IN6>(address)->sin6_addr.u.Word[i];
			if (i != 7) std::cout << ':';
		}
		std::cout << std::dec << '\n';
	}
	else if (address->sa_family == AF_INET) {
		std::cout << "IPv4: ";
		unsigned char* addr = reinterpret_cast<unsigned char*>(&reinterpret_cast<PSOCKADDR_IN>(address)->sin_addr.S_un);
		for (int i = 0; i < 4; ++i) {
			std::cout << addr[i];
			if (i != 7) std::cout << '.';
		}
		std::cout << '\n';
	}
	else {
		std::cout << "Cant pasre address\n";
	}
}