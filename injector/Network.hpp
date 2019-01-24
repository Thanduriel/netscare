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

#include "../src/utils.hpp"
#include "../server/Asn.hpp"
#include "../server/Messages.hpp"
#include "Gui.hpp"

namespace fs = std::filesystem;

class Client {
	HINTERNET _hIntSession;
	HINTERNET _hHttpSession;
	bool MakeRequest(std::vector<unsigned char>& res, std::vector<unsigned char>& msg, const char* subPage = "test1/");
	unsigned int _userId;
	bool _bLogIn;
	std::unordered_map<std::string, std::uint8_t> _mapPicId;
	
	struct LoadedPic { std::uint8_t uId, eId; };
	// std::unordered_map<LoadedPic, std::uint8_t> _mapLoadedId;
public:
	Client(const char* hostname = "localhost", unsigned int port = 9090) : _userId{ 0 }, _bLogIn{ false } {
		_hIntSession = InternetOpenA(_T("Client"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		_hHttpSession = InternetConnectA(_hIntSession, _T(hostname), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	}
	~Client() {
		InternetCloseHandle(_hHttpSession);
		InternetCloseHandle(_hIntSession);
	}
	bool isOnline() { return _bLogIn; }
	bool Login(const wchar_t* username);
	bool SendPicture(ScareEventCp& eventCp);
};
