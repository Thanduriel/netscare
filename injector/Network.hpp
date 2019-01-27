#pragma once

#include <Windows.h>
#include <wininet.h>
#pragma comment( lib, "wininet" )
#include <tchar.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <string>
#include <memory>
#include <cassert>

#include "../src/utils.hpp"
#include "../server/Asn.hpp"
#include "../server/Types.hpp"
#include "../server/Messages.hpp"
#include "Gui.hpp"

namespace fs = std::filesystem;

struct Host {
	std::wstring name;
	enum TYPE { IPV4, IPV6 } type;
	unsigned char address[16];
};

class Client {
	HINTERNET _hIntSession;
	HINTERNET _hHttpSession;
	bool MakeRequest(std::vector<unsigned char>& res, std::vector<unsigned char>& msg, const char* subPage = "/");
	unsigned int _userId;
	bool _bLogIn;
	std::unordered_map<std::string, std::uint8_t> _mapPicId;

	
	struct LoadedPic { std::uint8_t uId, picId; };
	// std::unordered_map<LoadedPic, std::uint8_t> _mapLoadedId;

	std::vector<Address>& _addresses;
	std::vector<std::unique_ptr<Command>> _commandQueue;
	bool SendPicture(ScareEventCp& eventCp);
	std::vector<ScareEventCp*> eventBuffer;
public:
	Client(std::vector<Address>& addresses, const char* hostname = "192.168.27.186", unsigned int port = 80) : _userId{ 0 }, _bLogIn{ false }, _addresses{ addresses}  {
		_hIntSession = InternetOpenA(_T("Client"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		_hHttpSession = InternetConnectA(_hIntSession, _T(hostname), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	}
	~Client() {
		InternetCloseHandle(_hHttpSession);
		InternetCloseHandle(_hIntSession);
	}
	bool IsOnline() { return _bLogIn; }
	bool Login(const wchar_t* username);
	bool AddEvent(ScareEventCp& eventCp);
	void TriggerEvent(int eventId) { _commandQueue.emplace_back(std::make_unique<TriggerEventCommand>(eventId)); }
	bool Update();
	bool SearchServer(std::vector<std::string>);
};
