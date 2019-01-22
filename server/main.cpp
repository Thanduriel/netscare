#include "Network.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <io.h>


// http resquets GET -> Download side from netscare
// http POST string:login,OctasString:Name,STRUCT:publicKey -> OctaString:msg ~> string:denied|access,Struct:symKey,Struct:macKey,Integer:uid
// http POST Integer:uid,STRUCT:macTag,OctaString:msg~>Integer:hash,struct:{string:pic, Intgeer:id, OcatString:ddsfile}|{string:update, struct{string:prep, I:target, I:picId, I:eventId}|struct{string:fire, I:target, I:eventId} }
// -> Strcut:mactag,OcatString:msg~>Integer:hash,struct{OcatSTring:tickets, BisString:eventPrep(y/n)|eventExecuted(y/n)}
// http POST Integer:uuid,Struct:maxTag, OCatSTring:msg~>Integer:hash,strcut:{string:names, OctaSTring:ArrayOfNames in wchar o terminiert}

constexpr int MAX_PICTURES_PER_USER = 20;
struct Event {
	std::uint8_t target, picId;
	Event() :target{ 0 }, picId{ 0 } {}
	Event(std::uint8_t target, std::uint8_t picId) : target{ target }, picId{ picId } {}
	enum STATE {NOT_SET, SETTED, PIC_LOADED, PREPARED, EXECUTED};
};
class User {
	std::wstring name;
	static int num;
	std::size_t ecexuted;
	std::vector<Event> events;
	std::vector<Event::STATE> states;
public:
	const uint8_t id;
	User(wchar_t *name) : id{ static_cast<std::uint8_t>(num++) }, name{ name }, events{ }, states{ }, ecexuted{ 0 } {
		if (num > 255) std::cerr << "To many User\n";
	}
	const std::wstring& getName() const { return name; }
	void addEvent(std::uint8_t target, std::uint8_t picId, std::uint8_t id, bool picLoaded) {
		if (id > events.size()) {
			events.resize(id);
			states.resize(id);
		}
		events[id] = Event(target, picId);
		states[id] = picLoaded ? Event::STATE::PIC_LOADED : Event::STATE::SETTED;
	}
	void loadPic(std::uint8_t picId) {
		for (std::size_t i = 0; i < events.size(); ++i) {
			if (states[i] != Event::STATE::SETTED) continue;
			if (events[i].picId == picId) states[i] = Event::STATE::PIC_LOADED;
		}
	}
};

int User::num = 0;

class Tickets {
	std::uint8_t *_data;
	int _amtUser;
	int _capacety;
public:
	Tickets(int aproxUsers) : _data{ new std::uint8_t[aproxUsers * aproxUsers] }, _amtUser{ 0 }, _capacety{aproxUsers} {
		memset(_data, 0, aproxUsers * aproxUsers * sizeof(std::uint8_t));
	}
	~Tickets() { delete[] _data; }
	void realloc(int size) {
		if (size <= _capacety) return;
		std::uint8_t *nData = new std::uint8_t[size * size];
		memcpy(nData, _data, _capacety * _capacety * sizeof(std::uint8_t));
		int d = size - _capacety;
		memset(nData + (_capacety * _capacety), 0, d * d * sizeof(std::uint8_t));
		delete[] _data;
		_data = nData;
		_capacety = size;
	}
	void addUser() {
		if (++_amtUser > _capacety) realloc(static_cast<int>(_capacety * 1.5f));
	}
	const std::uint8_t* getTickets(std::uint8_t userId) const {
		return _data + _capacety * userId;
	}
	std::uint8_t getTicket(std::uint8_t userId, std::uint8_t targetId) const { return _data[userId * _capacety + targetId]; }
	// user with -1 termenated list of user wich tickets schould be trade
	enum TRASNACTION { SUUCCESS, NOT_ENOUGH, TOO_MANY};
	TRASNACTION transaction(std::uint8_t u1, std::uint8_t u2, const std::uint8_t* user, const std::uint8_t* amount, std::size_t amt) {
		TRASNACTION res = SUUCCESS;
		std::uint8_t *user1 = _data + u1 * _capacety;
		std::uint8_t *user2 = _data + u2 * _capacety;
		for (std::size_t i = 0; i < amt; ++i) {
			if (amount[i] < 0) {
				if (u1 != user[i] && user1[user[i]] + amount[i] < 0) { res = NOT_ENOUGH; break; }
				if (user2[user[i]] - amount[i] > 255) { res = TOO_MANY; break; }
			} else {
				if (user1[user[i]] + amount[i] > 255) { res = TOO_MANY; break; }
				if (u2 != user[i] && user2[user[i]] - amount[i] < 0) { res = NOT_ENOUGH; break; }
			}
		}
		if (res != SUUCCESS) return res;
		for (std::size_t i = 0; i < amt; ++i) {
			if (user[i] != u1) user1[user[i]] += amount[i];
			if (user[i] != u2) user2[user[i]] -= amount[i];
		}
		return res;
	}
};

int __cdecl wmain(int argc, wchar_t **argv) {
	if (argc < 2) {
		wprintf(L"%s: <Url1> [Url2] ... \n", argv[0]);
		return -1;
	}
	NetScareServer server(argc, argv);

	Tickets tickets(20);
	std::vector<User> users{};
	
	NetScareServer::Action action;
	do {
		Sleep(100);
		server.updateServer(action);
		switch (action.type) {
		case NetScareServer::Action::USER_NEW:
			users.push_back(User(reinterpret_cast<wchar_t*>(action.data)));
			delete[] action.data;
			break;
		case NetScareServer::Action::PIC_LOAD: {
			std::uint8_t user = action.data[0], pId = action.data[1];
			std::size_t len = *reinterpret_cast<std::size_t*>(action.data + 2);
			char fName[32];
			sprintf_s(fName, "%d_%d.dds", user, pId);
			std::ofstream file(fName);
			file.write(action.data + 2 + sizeof(std::size_t),len);
			file.close();
			users[user].loadPic(pId);
		}	break;
		case NetScareServer::Action::EVENT_ADD: {
			char name[25];
			sprintf_s(name, "%d_%d.dds", action.data[0], action.data[1]);
			users[action.data[0]].addEvent(action.data[0], action.data[1], *reinterpret_cast<std::uint8_t*>(action.data + 2), _access(name, 0));
			delete[] action.data;
		}	break;
		}
	} while (true);
}