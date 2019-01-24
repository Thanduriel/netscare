#include "Network.hpp"

namespace fs = std::filesystem;

bool Client::MakeRequest(std::vector<unsigned char>& res, std::vector<unsigned char>& msg, const char* subPage) {
	HINTERNET hHttpRequest = HttpOpenRequestA(_hHttpSession, _T("POST"), _T(subPage), NULL, "application/asn", NULL, INTERNET_FLAG_RELOAD, 0);
	TCHAR* szHeader = _T("Content-Type: application/asn");
	
	ASNObject *asn = ASNObject::DecodeAsn(msg.data(), msg.size());
	for (unsigned long pos = 0; pos < msg.size(); pos += asn->GetSize(), ++asn) {
		if (asn->GetType() != ASNObject::UTF8String) std::cout << *asn;
		else std::wcout << L"UTF8: " << asn->DecodeUTF8() << L'\n';
	}

	
	if (!HttpSendRequestA(hHttpRequest, szHeader, _tcslen(szHeader), msg.data(), msg.size())) {
		Utils::ShowError("Erroro when send", GetLastError());
		// printf("Error when send: %ul", GetLastError());
		return false;
	}
	
	unsigned char szBuffer[1024];
	DWORD dwRead = 0;
	res.resize(0);
	while (InternetReadFile(hHttpRequest, szBuffer, sizeof(szBuffer) - 1, &dwRead) && dwRead) {
		res.insert(res.end(), szBuffer, szBuffer + dwRead);
		dwRead = 0;
	}

	ASNObject* asnO = ASNObject::DecodeAsn(res.data(), res.size());
	for (unsigned long pos = 0; pos < res.size(); pos += asnO->GetSize(), ++asnO) {
		std::cout << *asnO;
	}

	InternetCloseHandle(hHttpRequest);
	return true;
}

bool Client::SendPicture(ScareEventCp& eventCp) {
	fs::path path(eventCp.file);
	if (!_bLogIn) {
		MessageBox(NULL, "Please log to an server!", "User Error", MB_OK | MB_ICONWARNING);
		return false;
	} else if (!fs::exists(path)) {
		MessageBox(NULL, path.string().c_str(), "No File!", MB_OK | MB_ICONERROR);
	}

	enum STATE { CHECK, NOTHINGTODO, SUCCESS, FAILED, DEFECT } state = CHECK;

	std::unordered_map<std::string, std::uint8_t>::iterator pic = _mapPicId.find(path.string());
	int picId = 0;
	if (pic != _mapPicId.end()) state = STATE::NOTHINGTODO;
	if (state == STATE::CHECK) {
		picId = _mapPicId.size();

		std::ifstream file(path, std::ios::binary | std::ios::ate);
		std::streamoff size = file.tellg();
		file.seekg(std::ios_base::beg);
		std::vector<unsigned char> data(static_cast<std::size_t>(size));
		file.read(reinterpret_cast<char*>(data.data()), data.size());
		file.close();


		unsigned long decode[] = {
			ASNObject::EncodingSize(NM_LOADPIC),
			ASNObject::EncodingSize(_userId),
			ASNObject::EncodingSize(picId),
			ASNObject::EncodingSize(data)
		};
		if (decode[2] > 3) {
			MessageBox(NULL, "PicId too large!", "OverflowEror", MB_OK | MB_ICONWARNING);
			return false;
		}

		unsigned long len = decode[0] + decode[1] + decode[2] + decode[3], pos = 0;
		std::vector<unsigned char> szReq(len, 0);
		ASNObject::EncodeAsnPrimitives(NM_LOADPIC, szReq.data() + pos);
		pos += decode[0];
		ASNObject::EncodeAsnPrimitives(_userId, szReq.data() + pos);
		pos += decode[1];
		ASNObject::EncodeAsnPrimitives(picId, szReq.data() + pos);
		pos += decode[2];
		ASNObject::EncodeAsnPrimitives(data, szReq.data() + pos);

		std::vector<unsigned char> res;
		if (!MakeRequest(res, szReq)) {
			MessageBox(NULL, "RequestFailed", "NetworkError", MB_OK | MB_ICONERROR);
			return false;
		}

		ASNObject *itr, *asn = ASNObject::DecodeAsn(res.data(), res.size());
		itr = asn;
		int i = 0;
		for (unsigned long pos = 0; pos < res.size(); pos += itr->GetSize(), ++itr, ++i) {
			switch (i)
			{
			case 0:
				if (itr->GetType() != ASNObject::ASCISTRING) { state = DEFECT; break; }
				if (strcmp(itr->DecodeString(), NM_FAILED) == 0) { state = FAILED; break; }
				if (strcmp(itr->DecodeString(), NM_SUCCESS) == 0) { state = SUCCESS; break; }
				state = DEFECT; break;
			case 1:
				if (itr->GetType() != ASNObject::ASCISTRING) { state = DEFECT; break; }
				MessageBox(NULL, itr->DecodeString(), "UplaodPicture Error", MB_OK | MB_ICONWARNING);
				return false;
			default: state = DEFECT;
			}
			if (state == DEFECT) {
				MessageBox(NULL, "Package defekt", "Network Error", MB_OK | MB_ICONERROR);
				return false;
			}
		}
	}
	
	if (state == SUCCESS || state == NOTHINGTODO) {
		if (state == SUCCESS) 
			_mapPicId.insert(std::make_pair(path.string(), picId));
		eventCp.changeState(ScareEvent::STATE::SETTET);
		return true;
	}
	return false;
} 

bool Client::Login(const wchar_t* username) {

	if (_bLogIn) {
		MessageBox(NULL, "You Already Login", "User Error", MB_OK | MB_ICONWARNING);
		return false;
	}

	unsigned long decode[] = {
		ASNObject::EncodingSize(NM_LOGIN),
		ASNObject::EncodingSize(username),
	};
	unsigned long len = decode[0] + decode[1];
	std::vector<unsigned char> szReq(len, 0);
	ASNObject::EncodeAsnPrimitives(NM_LOGIN, szReq.data());
	ASNObject::EncodeAsnPrimitives(username, szReq.data() + decode[0]);
	
	std::vector<unsigned char> res;
	if (!MakeRequest(res, szReq)) {
		MessageBox(NULL, "Requet Faield", "NetworkError", MB_OK | MB_ICONERROR);
		return false;
	}
	
	ASNObject *itr, *asn = ASNObject::DecodeAsn(res.data(), res.size());
	itr = asn;
	int id = 0;
	enum STATE { CHECK, FAILED, SUCCESS, DEFECT } state = CHECK;
	for (unsigned long pos = 0; pos < res.size(); pos += itr->GetSize(), ++itr, ++id) {
		switch (id) {
		case 0:
			if (itr->GetType() != ASNObject::ASCISTRING) { state = DEFECT; break; }
			if (!strcmp(itr->DecodeString(), NM_FAILED)) { state = FAILED; break; }
			if (!strcmp(itr->DecodeString(), NM_SUCCESS)) { state = SUCCESS; break; }
			state = DEFECT; break;
		case 1:
			if (state == FAILED) {
				if (itr->GetType() != ASNObject::ASCISTRING) { state = DEFECT; break; }
				MessageBox(NULL, itr->DecodeString(), "Login Error", MB_OK | MB_ICONWARNING);
				return false;
			} else if (state == SUCCESS) {
				if (itr->GetType() != ASNObject::TYPE::INTEGER) { state = DEFECT; break; }
				_userId = itr->DecodeInteger();
			}
			break;
		default: state = DEFECT;
		}
		if (state == DEFECT) {
			MessageBox(NULL, "Package defekt", "Network Error", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	if (state == SUCCESS) {
		_bLogIn = true;
		return true;
	}
	return false;
}

bool Client::Update() {
	unsigned long len = 0,
		offset = ASNObject::EncodingSize(NM_UPDATE);
	len += offset;
	len += ASNObject::EncodingSize(_commandQueue);
	
	std::vector<unsigned char> szReq(len);
	ASNObject::EncodeAsnPrimitives(NM_UPDATE, szReq.data());
	ASNObject::EncodeSequence(_commandQueue, szReq.data() + offset);

	if (!MakeRequest(szReq, szReq)) {
		MessageBox(NULL, "Requet Faield", "NetworkError", MB_OK | MB_ICONERROR);
		return false;
	}

	// TODO:: repond handling
	return true;
}