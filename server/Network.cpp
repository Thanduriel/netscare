#include "Network.hpp"

int User::num = 0;

NetScareServer::NetScareServer(int urlC, wchar_t **urls, std::vector<User>& users) : urlC{ urlC }, urls{ urls }, hReqQueue{ NULL }, urlAdded{ 0 }, bPending{ false }, overlapped{ 0 }, users{ users } {
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

DWORD NetScareServer::updateServer(Action& action, BOOL wait) {
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
	ULONG           TempFileBytesWritten;
	HANDLE          hTempFile;
	TCHAR           szTempName[MAX_PATH + 1];
	CHAR            szContentLength[MAX_ULONG_STR];
	HTTP_DATA_CHUNK dataChunk;
	ULONG           TotalBytesRead = 0;

	BytesRead = 0;
	hTempFile = INVALID_HANDLE_VALUE;

	//
	// Allocate space for an entity buffer. Buffer can be increased 
	// on demand.
	//
	EntityBufferLength = 2048;
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
		// The entity body is sent over multiple calls. Collect 
		// these in a file and send back. Create a temporary 
		// file.
		//

		if (GetTempFileName(
			L".",
			L"New",
			0,
			szTempName
		) == 0)
		{
			result = GetLastError();
			wprintf(L"GetTempFileName failed with %lu \n", result);
			goto Done;
		}

		hTempFile = CreateFile(
			szTempName,
			GENERIC_READ | GENERIC_WRITE,
			0,                  // Do not share.
			NULL,               // No security descriptor.
			CREATE_ALWAYS,      // Overrwrite existing.
			FILE_ATTRIBUTE_NORMAL,    // Normal file.
			NULL
		);

		if (hTempFile == INVALID_HANDLE_VALUE)
		{
			result = GetLastError();
			wprintf(L"Cannot create temporary file. Error %lu \n",
				result);
			goto Done;
		}

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
					WriteFile(
						hTempFile,
						pEntityBuffer,
						BytesRead,
						&TempFileBytesWritten,
						NULL
					);
				}
				break;

			case ERROR_HANDLE_EOF: {
				if (BytesRead != 0)
				{
					TotalBytesRead += BytesRead;
					WriteFile(
						hTempFile,
						pEntityBuffer,
						BytesRead,
						&TempFileBytesWritten,
						NULL
					);
				}
				unsigned char * data = new unsigned char[TotalBytesRead];
				DWORD res = 0;

				if (SetFilePointer(hTempFile, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
					wprintf(L"ERROR: %ud", GetLastError());



				ReadFile(hTempFile, data, TotalBytesRead, &res, NULL);
				ASNObject *asn = ASNObject::DecodeAsn(data, TotalBytesRead);
				std::uint8_t x, y;
				if (asn->GetType() == ASNObject::ASCISTRING) {
					if (strcmp(asn[0].DecodeString(), "calc") == 0) {
						x = static_cast<std::uint8_t>(asn[1].DecodeInteger());
						y = static_cast<std::uint8_t>(asn[2].DecodeInteger());
					} else std::cout << "No calc operation: " << asn->DecodeString() << '\n';
				} else std::cout << "No String Type: " << asn->GetType() << '\n';
				char msg[] = {0x13, 0x04, 'r', 'e', 's', 0, 0x02, 0x01, x + y};
				SetFilePointer(hTempFile, 0, 0, FILE_BEGIN);
				if (!WriteFile(hTempFile, msg, 10, &res, NULL))
					std::cout << "ERROR: " << GetLastError() << '\n';
				sprintf_s(szContentLength, MAX_ULONG_STR, "%lu", 10);
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

				//
				// Send entity body from a file handle.
				//
				/* dataChunk.DataChunkType = HttpDataChunkFromFileHandle;
				dataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
				dataChunk.FromFileHandle.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;
				dataChunk.FromFileHandle.FileHandle = hTempFile; */

				dataChunk.DataChunkType = HttpDataChunkFromMemory;
				dataChunk.FromMemory.pBuffer = msg;
				dataChunk.FromMemory.BufferLength = 13;

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
		// This request does not have an entity body.
		//

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

	if (pEntityBuffer)
	{
		FREE_MEM(pEntityBuffer);
	}

	if (INVALID_HANDLE_VALUE != hTempFile)
	{
		CloseHandle(hTempFile);
		DeleteFile(szTempName);
	}

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

