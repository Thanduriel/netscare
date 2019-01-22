#include "Types.hpp"

Tickets::Tickets(int aproxUsers) : _data{ new std::uint8_t[aproxUsers * aproxUsers] }, _amtUser{ 0 }, _capacity{ aproxUsers } {
	memset(_data, 0, aproxUsers * aproxUsers * sizeof(std::uint8_t));
}
Tickets::~Tickets() { delete[] _data; }
void Tickets::realloc(int size) {
	if (size <= _capacity) return;
	std::uint8_t *nData = new std::uint8_t[size * size];
	memcpy(nData, _data, _capacity * _capacity * sizeof(std::uint8_t));
	int d = size - _capacity;
	memset(nData + (_capacity * _capacity), 0, d * d * sizeof(std::uint8_t));
	delete[] _data;
	_data = nData;
	_capacity = size;
}
void Tickets::addUser() {
		if (++_amtUser > _capacity) realloc(static_cast<int>(_capacity * 1.5f));
	}
const std::uint8_t* Tickets::getTickets(std::uint8_t userId) const {
	return _data + _capacity * userId;
}
std::uint8_t Tickets::getTicket(std::uint8_t userId, std::uint8_t targetId) const {
	return _data[userId * _capacity + targetId];
}

Tickets::TRASNACTION Tickets::transaction(std::uint8_t u1, std::uint8_t u2, const std::uint8_t* user, const std::uint8_t* amount, size_t amt) {
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

User::User(wchar_t *name) : id{ static_cast<std::uint8_t>(num++) }, name{ name }, events{}, states{}, ecexuted{ 0 } {
		if (num > 255) MessageBoxA(NULL, "To many User\n", "memeory Error", MB_OK | MB_ICONERROR);
	}
const std::wstring& User::getName() const { return name; }
void User::addEvent(std::uint8_t target, std::uint8_t picId, std::uint8_t id, bool picLoaded) {
	if (id > events.size()) {
		events.resize(id);
		states.resize(id);
	}
	events[id] = Event(target, picId);
	states[id] = picLoaded ? Event::STATE::PIC_LOADED : Event::STATE::SETTED;
}
void User::updatePic(std::uint8_t picId) {
		for (std::size_t i = 0; i < events.size(); ++i) {
			if (states[i] != Event::STATE::PREPARED) continue;
			if (events[i].picId == picId) states[i] = Event::STATE::PIC_LOADED;
		}
	}
void User::loadPic(std::uint8_t picId) {
	for (std::size_t i = 0; i < events.size(); ++i) {
		if (states[i] != Event::STATE::SETTED) continue;
		if (events[i].picId == picId) states[i] = Event::STATE::PIC_LOADED;
	}
}