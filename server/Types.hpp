#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>

#include "Asn.hpp"

namespace fs = std::filesystem;

struct Event {
	std::uint8_t target, picId;
	Event() :target{ 0 }, picId{ 0 } {}
	Event(std::uint8_t target, std::uint8_t picId) : target{ target }, picId{ picId } {}
	enum STATE { NOT_SET, SETTED, PIC_LOADED, PREPARED, EXECUTED };
};

class Tickets {
	typedef unsigned long size_t;
	std::uint8_t *_data;
	int _amtUser;
	int _capacity;
public:
	Tickets(int aproxUsers);
	~Tickets();
	void Realloc(int size);
	void AddUser();
	std::size_t GetUserSize() const { return _amtUser; }
	const std::uint8_t* GetTickets(std::uint8_t userId) const;
	std::uint8_t GetTicket(std::uint8_t userId, std::uint8_t targetId) const;
	enum TRASNACTION { SUUCCESS, NOT_ENOUGH, TOO_MANY };
	TRASNACTION Transaction(std::uint8_t u1, std::uint8_t u2, const std::uint8_t* user, const std::uint8_t* amount, size_t amt);
};

class User {
	std::wstring name;
	static int num;
	std::vector<std::pair<std::uint8_t, std::uint8_t>> _pics;
	std::vector<Event> events;
	std::vector<Event::STATE> states;
public:
	std::size_t executedCommands;
	const uint8_t id;
	User(const wchar_t *name);
	User(const char* name);
	const std::wstring& GetName() const;
	void AddEvent(std::uint8_t target, std::uint8_t picId, std::uint8_t id, bool picLoaded);
	const Event& GetEvent(std::uint8_t id) { 
		if (id >= events.size()) MessageBoxA(NULL, "NOT GOOD", "out of bound", MB_OK | MB_ICONERROR);
		return events[id]; 
	}
	void UpdatePic(std::uint8_t picId);
	void LoadPic(std::uint8_t picId);
};

class Command {
public:
	enum TYPE { TRIGGEREVENT, ADD_USER, DOWNLAOD_PIC, ADDEVENT, UPDATE_TICKETS };
	enum DECODERESULT { DECODED, CORRUPTED };
	TYPE GetType() const { return _type; }
	DECODERESULT GetState() const { return _decodeState; }
	virtual unsigned long EncodeSize() const { return 0; };			// return Space for Encoding
	virtual void Encode(unsigned char* msg) const {};	// encodes in space
	static std::unique_ptr<Command> Decode(const ASNObject::ASNDecodeReturn& asn);
	Command(TYPE type) : _type{ type }, _decodeState{ CORRUPTED } {}
protected:
	mutable unsigned long _size{ 0 };
	static constexpr char * const IDENTIFYER[] = {
		"triggerEvent",
		"addUser",
		"downloadPic",
		"addEvent",
		"updateTickets"
	};
	Command(TYPE type, bool state) : _type{ type }, _decodeState{ state ? DECODED : CORRUPTED } {}
	DECODERESULT _decodeState;
private:
	TYPE _type;
	static constexpr unsigned long MAX_NAME{ 32 };
};

class TriggerEventCommand : public Command {
	int _eventId;
	mutable unsigned long _idSize{ 0 };
public:
	TriggerEventCommand(int eventId) : Command{ TRIGGEREVENT, true }, _eventId{ eventId } {}
	int GetEventId() const { return _eventId; }
	unsigned long EncodeSize() const override;
	void Encode(unsigned char* msg) const override;
};

class AddEventCommand : public Command{
	std::uint8_t _target;
	std::uint8_t _pId;
	int _eventId;
	mutable unsigned long _idSize{ 0 };
public:
	struct Event {
		Event(std::uint8_t targte, std::uint8_t pId, int eventId) : target{ targte }, pId{ pId }, eventId{ eventId } {}
		std::uint8_t target, pId;
		int eventId;
	};
	Event GetEvent() { return Event(_target, _pId, _eventId); }
	AddEventCommand(std::uint8_t target, std::uint8_t pId, int eventId) : Command{ ADDEVENT, true }, _target{ target }, _pId{ pId }, _eventId{ eventId } {}
	unsigned long EncodeSize() const override;
	void Encode(unsigned char* msg) const override;
};

class AddUserCommand : public Command {
	std::uint8_t _id;
	wchar_t _username[32];
	mutable unsigned long _idSize{ 0 };
	static constexpr unsigned long _numSize{ 3 };
public:
	AddUserCommand(const wchar_t* username, std::uint8_t id) : Command{ ADD_USER, true }, _id{ id }{
		wcscpy_s(_username, username);
	}
	std::uint8_t GetId() const { return _id; }
	const wchar_t* GetUsername() const { return _username; }
	unsigned long EncodeSize() const override;
	void Encode(unsigned char* msg) const override;
};

class LoadPictureCommand : public Command {
	char _fileName[32];
	unsigned long _fileSize;
	const unsigned char* _data;
	mutable unsigned long _idSize{ 0 }, _size{ 0 };
public:
	bool SavePicture(fs::path path);
	LoadPictureCommand(const char* fileName, unsigned long size, const unsigned char* data) : Command{DOWNLAOD_PIC, false}, _fileSize{ size }, _data{ data } {
		strcpy_s(_fileName, fileName);
	}
	LoadPictureCommand(const char* fileName) : Command{ DOWNLAOD_PIC, true }, _data { nullptr } {
		strcpy_s(_fileName, fileName);
		fs::path path = fs::absolute(fs::path(fileName));
		if (!fs::exists(path)) {
			MessageBoxA(NULL, fs::absolute(path).string().c_str(), "Failed to open", MB_OK | MB_ICONERROR);
		}
		HANDLE hFile = CreateFileA(path.string().c_str(), GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile == NULL)
			MessageBoxA(NULL, std::to_string(GetLastError()).c_str(), "Can't open Files", MB_OK | MB_ICONERROR);
		_fileSize = GetFileSize(hFile, NULL);
		if (_fileSize == NULL) {
			MessageBoxA(NULL, std::to_string(GetLastError()).c_str(), "Can't get size", MB_OK | MB_ICONERROR);
		}
		CloseHandle(hFile);
		// if (off > ULONG_MAX) MessageBoxW(NULL, L"File To large", L"Handling Warning", MB_OK | MB_ICONWARNING);
	}
	unsigned long EncodeSize() const override;
	void Encode(unsigned char* msg) const override;
};

class UpdateTicketsCommand : public Command {
	mutable unsigned long _idSize{ 0 };
public:
	const std::uint8_t* const tickest;
	const std::size_t userAmt;
	UpdateTicketsCommand(const Tickets& tik, std::uint8_t uId) : Command{ UPDATE_TICKETS, true }, tickest{ tik.GetTickets(uId) }, userAmt{ tik.GetUserSize() } {}
	UpdateTicketsCommand(const std::uint8_t* tik, std::size_t amtUser) : Command{ UPDATE_TICKETS, true }, tickest{ tik }, userAmt{ amtUser } {}
	unsigned long EncodeSize() const override;
	void Encode(unsigned char* msg) const override;
};

