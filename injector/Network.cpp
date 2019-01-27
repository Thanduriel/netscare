#include "../../dependencies/DirectXTex/DirectXTex/DirectXTex.h"
#include "Network.hpp"

namespace fs = std::filesystem;

bool Client::MakeRequest(std::vector<unsigned char>& res, std::vector<unsigned char>& msg, const char* subPage) {
	HINTERNET hHttpRequest = HttpOpenRequestA(_hHttpSession, _T("POST"), _T(subPage), NULL, "application/asn", NULL, INTERNET_FLAG_RELOAD, 0);
	TCHAR* szHeader = _T("Content-Type: application/asn");
	
	if (!HttpSendRequestA(hHttpRequest, szHeader, _tcslen(szHeader), msg.data(), msg.size())) {
		DWORD error = GetLastError();
		Utils::ShowError(std::to_string(error).c_str(), error);
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

		std::vector<unsigned char> data;

		fs::path extension = path.extension();
		if (strcmp(extension.string().c_str(), ".dds") == 0
			|| strcmp(extension.string().c_str(), ".DDS") == 0) {
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			std::streamoff size = file.tellg();
			file.seekg(std::ios_base::beg);
			data.resize(static_cast<std::size_t>(size));
			file.read(reinterpret_cast<char*>(data.data()), data.size());
			file.close();
		} else {
			std::unique_ptr<DirectX::ScratchImage> image = std::make_unique<DirectX::ScratchImage>();
			HRESULT hr = DirectX::LoadFromWICFile( path.wstring().c_str(), DirectX::WIC_FLAGS_NONE, nullptr, *image);
			if (FAILED(hr)) {
				MessageBoxA(NULL, path.string().c_str(), "Cannot connvert File", MB_OK | MB_ICONERROR);
				return false;
			}
			DirectX::Blob blob{};
			hr = DirectX::SaveToDDSMemory(*(image->GetImages()), DirectX::WIC_FLAGS_NONE, blob);
			if (FAILED(hr)) {
				MessageBoxA(NULL, path.string().c_str(), "Cannot Save DDS", MB_OK | MB_ICONERROR);
				return false;
			}
			data.resize(blob.GetBufferSize());
			memcpy_s(data.data(), data.size(), blob.GetBufferPointer(), blob.GetBufferSize());
		}



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

		const ASNObject *itr,*asn = ASNObject::DecodeAsn(res.data(), res.size());
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
		eventBuffer.emplace_back(&eventCp);
		return true;
	}
	return false;
} 

bool Client::AddEvent(ScareEventCp& eventCp) {
	if (!SendPicture(eventCp)) return false;
	auto itr = _mapPicId.find(eventCp.file);
	if (itr == _mapPicId.end()) return false;
	_commandQueue.emplace_back(std::make_unique<AddEventCommand>(static_cast<std::uint8_t>(
			_addresses[eventCp.target].userId
		), itr->second, eventCp.id));
	return true;
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
		MessageBox(NULL, std::to_string(GetLastError()).c_str(), "NetworkError", MB_OK | MB_ICONERROR);
		return false;
	}
	
	const ASNObject *itr;
	ASNObject::ASNDecodeReturn asn = ASNObject::DecodeAsn(res.data(), res.size());
	itr = asn.objects;
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
	if (!_bLogIn) return false;
	unsigned long len = 0,
		offset[] = {
			ASNObject::EncodingSize(NM_UPDATE),
			ASNObject::EncodingSize(_userId)
		};
	len += offset[0] + offset[1];
	if (!_commandQueue.empty())
		len += ASNObject::EncodingSize<std::unique_ptr<Command>>(_commandQueue);
	
	std::vector<unsigned char> szReq(len);
	ASNObject::EncodeAsnPrimitives(NM_UPDATE, szReq.data());
	ASNObject::EncodeAsnPrimitives(_userId, szReq.data() + offset[0]);
	if (!_commandQueue.empty())
		ASNObject::EncodeSequence<std::unique_ptr<Command>>(_commandQueue, szReq.data() + offset[0] + offset[1]);

	if (!MakeRequest(szReq, szReq)) {
		MessageBox(NULL, "Requet Faield", "NetworkError", MB_OK | MB_ICONERROR);
		return false;
	}
	_commandQueue.resize(0);

	ASNObject::ASNDecodeReturn asn = ASNObject::DecodeAsn(szReq.data(), szReq.size());
	if (asn.len != 2
		|| asn.objects[0].GetType() != ASNObject::ASCISTRING
		|| asn.objects[1].GetType() != ASNObject::SEQUENCE) return false;
	ASNObject::ASNDecodeReturn cmds = asn.objects[1].DecodeASNObjscets();
	for (unsigned long i = 0; i < cmds.len; ++i) {
		if (cmds.objects[i].GetType() != ASNObject::SEQUENCE) continue;
		std::unique_ptr<Command> cmd = Command::Decode(cmds.objects[i].DecodeASNObjscets());
		switch (cmd->GetType()) {
		
		case Command::TYPE::ADD_USER: {
			AddUserCommand *aue = dynamic_cast<AddUserCommand*>(cmd.get());
			if (aue) { 
				if (aue->GetId() != _userId) _addresses.push_back(Address(aue->GetUsername(), aue->GetId(), 0));
			}
			else MessageBox(NULL, "Adress Update Failed", "Error", MB_OK | MB_ICONERROR);
		}	break;
		
		case Command::TYPE::TRIGGEREVENT:
			MessageBox(NULL, "Event Trigger", "Action", MB_OK);
			break;
		case Command::TYPE::EXECUTE_EVENT: {
			ExecutedCommand *ecm = dynamic_cast<ExecutedCommand*>(cmd.get());
			if (!ecm) MessageBox(NULL, "Cant Parse Triggerd Event", "Parse Error", MB_OK | MB_ICONERROR);
			else {
				for (std::vector<ScareEventCp*>::iterator itr = eventBuffer.begin(); itr < eventBuffer.end(); ++itr) {
					if ((*itr)->id == ecm->eventId) {
						(*itr)->changeState(ScareEvent::EXECUTED);
						eventBuffer.erase(itr);
						break;
					}
				}
			}
		}	break;
		case Command::TYPE::DOWNLAOD_PIC: {
			LoadPictureCommand * lpc = dynamic_cast<LoadPictureCommand*>(cmd.get());
			if (!lpc) MessageBox(NULL, "Download Picture", "Failed", MB_OK | MB_ICONERROR);
			else {
				// MessageBox(NULL, "Load", "Load File", MB_OK);
				if (!lpc->SavePicture(".")) MessageBox(NULL, ".", "Failed to save File:", MB_OK | MB_ICONERROR);
			}
		}	break;
		
		case Command::TYPE::UPDATE_TICKETS: {
			UpdateTicketsCommand *utc = dynamic_cast<UpdateTicketsCommand*>(cmd.get());
			if (!utc) std::cout << "Failed to Decode Update Tickets\n";
			else {
				for (auto& a : _addresses) {
					if (a.userId >= 0 && static_cast<std::size_t>(a.userId) < utc->userAmt)
						a.tickets = utc->tickest[a.userId];
				}
			}
		}	break;
		}
	}
	return true;
}