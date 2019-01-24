#include "Types.hpp"

int User::num = 0;

Tickets::Tickets(int aproxUsers) : _data{ new std::uint8_t[aproxUsers * aproxUsers] }, _amtUser{ 0 }, _capacity{ aproxUsers } {
	memset(_data, 0, aproxUsers * aproxUsers * sizeof(std::uint8_t));
}
Tickets::~Tickets() { delete[] _data; }
void Tickets::Realloc(int size) {
	if (size <= _capacity) return;
	std::uint8_t *nData = new std::uint8_t[size * size];
	memcpy(nData, _data, _capacity * _capacity * sizeof(std::uint8_t));
	int d = size - _capacity;
	memset(nData + (_capacity * _capacity), 0, d * d * sizeof(std::uint8_t));
	delete[] _data;
	_data = nData;
	_capacity = size;
}
void Tickets::AddUser() {
		if (++_amtUser > _capacity) Realloc(static_cast<int>(_capacity * 1.5f));
	}
const std::uint8_t* Tickets::GetTickets(std::uint8_t userId) const {
	return _data + _capacity * userId;
}
std::uint8_t Tickets::GetTicket(std::uint8_t userId, std::uint8_t targetId) const {
	return _data[userId * _capacity + targetId];
}

Tickets::TRASNACTION Tickets::Transaction(std::uint8_t u1, std::uint8_t u2, const std::uint8_t* user, const std::uint8_t* amount, size_t amt) {
	TRASNACTION res = SUUCCESS;
	std::uint8_t *user1 = _data + u1 * _capacity;
	std::uint8_t *user2 = _data + u2 * _capacity;
	for (size_t i = 0; i < amt; ++i) {
		if (amount[i] < 0) {
			if (u1 != user[i] && user1[user[i]] + amount[i] < 0) { res = NOT_ENOUGH; break; }
			if (user2[user[i]] - amount[i] > 255) { res = TOO_MANY; break; }
		}
		else {
			if (user1[user[i]] + amount[i] > 255) { res = TOO_MANY; break; }
			if (u2 != user[i] && user2[user[i]] - amount[i] < 0) { res = NOT_ENOUGH; break; }
		}
	}
	if (res != SUUCCESS) return res;
	for (size_t i = 0; i < amt; ++i) {
		if (user[i] != u1) user1[user[i]] += amount[i];
		if (user[i] != u2) user2[user[i]] -= amount[i];
	}
	return res;
}

User::User(const wchar_t *name) : id{ static_cast<std::uint8_t>(num++) }, name{ name }, events{}, states{}, ecexuted{ 0 } {
	if (num > 255) MessageBoxA(NULL, "To many User\n", "memeory Error", MB_OK | MB_ICONERROR);
}
User::User(const char *name) : id{ static_cast<std::uint8_t>(num++) }, events{}, states{}, ecexuted{ 0 }, name{name, name + strlen(name) + 1} {
	if (num > 255) MessageBoxA(NULL, "To many User\n", "memeory Error", MB_OK | MB_ICONERROR);
}

const std::wstring& User::GetName() const { return name; }
void User::AddEvent(std::uint8_t target, std::uint8_t picId, std::uint8_t id, bool picLoaded) {
	if (id > events.size()) {
		events.resize(id);
		states.resize(id);
	}
	events[id] = Event(target, picId);
	states[id] = picLoaded ? Event::STATE::PIC_LOADED : Event::STATE::SETTED;
}
void User::UpdatePic(std::uint8_t picId) {
		for (std::size_t i = 0; i < events.size(); ++i) {
			if (states[i] != Event::STATE::PREPARED) continue;
			if (events[i].picId == picId) states[i] = Event::STATE::PIC_LOADED;
		}
	}
void User::LoadPic(std::uint8_t picId) {
	for (std::size_t i = 0; i < events.size(); ++i) {
		if (states[i] != Event::STATE::SETTED) continue;
		if (events[i].picId == picId) states[i] = Event::STATE::PIC_LOADED;
	}
}

std::unique_ptr<Command> Command::Decode(ASNObject::ASNDecodeReturn& asn) {
	if (asn.len < 1 || asn.objects[0].GetType() != ASNObject::ASCISTRING) return nullptr;
	const char* type = asn.objects[0].DecodeString();
	if (strcmp(type, Command::IDENTIFYER[ADD_USER])) {
		if (asn.len != 3
			|| asn.objects[1].GetType() != ASNObject::UTF8String
			|| asn.objects[1].GetDataSize() / sizeof(wchar_t) > MAX_NAME
			|| asn.objects[2].GetType() != ASNObject::INTEGER) return std::make_unique<Command>(ADD_USER);
		int id = asn.objects[2].DecodeInteger();
		if (id > 255) return std::make_unique<Command>(ADD_USER);
		return std::make_unique<AddUserCommand>(asn.objects[1].DecodeUTF8(), static_cast<std::uint8_t>(id));
	} else if (strcmp(type, Command::IDENTIFYER[TRIGGEREVENT])){
		if (asn.len != 2
			|| asn.objects[1].GetType() != ASNObject::INTEGER) return std::make_unique<Command>(TRIGGEREVENT);
		return std::make_unique<TriggerEventCommand>(asn.objects[1].DecodeInteger());
	}
	return nullptr;
}

unsigned long AddUserCommand::EncodeSize() {
	if (_idSize && _size) return _size;
	_idSize = ASNObject::EncodingSize(Command::IDENTIFYER[GetType()]);
	_size = _idSize + ASNObject::EncodingSize(_username) + _numSize;
	return _size;
}

void AddUserCommand::Encode(unsigned char* msg) {
	unsigned long offset = _idSize;
	ASNObject::EncodeAsnPrimitives(Command::IDENTIFYER[GetType()], msg);
	ASNObject::EncodeAsnPrimitives(_id, msg + offset);
	offset += _numSize;
	ASNObject::EncodeAsnPrimitives(_username, msg + offset);
}

unsigned long TriggerEventCommand::EncodeSize() {
	if (_idSize && _size) return _size;
	_idSize = ASNObject::EncodingSize(Command::IDENTIFYER[GetType()]);
	_size = _idSize + ASNObject::EncodingSize(_eventId);
	return _size;
}

void TriggerEventCommand::Encode(unsigned char* msg) {
	unsigned long offset = _idSize ? _idSize : ASNObject::EncodingSize(Command::IDENTIFYER[GetType()]);
	ASNObject::EncodeAsnPrimitives(Command::IDENTIFYER[GetType()], msg);
	ASNObject::EncodeAsnPrimitives(_eventId, msg + offset);
}

unsigned long LoadPictureCommand::EncodeSize() {
	if (_idSize && _size) return _size;
	_idSize = ASNObject::EncodingSize(Command::IDENTIFYER[GetType()]);
	_size = _idSize + _fileSize + ASNObject::EncodeHeaderSize(_fileSize);
	return _size;
}

void LoadPictureCommand::Encode(unsigned char* msg) {
	unsigned long offset = _idSize ? _idSize : ASNObject::EncodingSize(Command::IDENTIFYER[GetType()]);
	ASNObject::EncodeAsnPrimitives(Command::IDENTIFYER[GetType()], msg);
	std::ifstream file(_fileName, std::ios::binary | std::ios::app);
	std::vector<unsigned char> data(_fileSize);
	file.read(reinterpret_cast<char*>(data.data()), _fileSize);
	file.close();
	ASNObject::EncodeAsnPrimitives(data, msg + offset);
}

