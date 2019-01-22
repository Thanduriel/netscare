#include <windows.h>
#include <cstdint>
#include <vector>

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
	void realloc(int size);
	void addUser();
	const std::uint8_t* getTickets(std::uint8_t userId) const;
	std::uint8_t getTicket(std::uint8_t userId, std::uint8_t targetId) const;
	enum TRASNACTION { SUUCCESS, NOT_ENOUGH, TOO_MANY };
	TRASNACTION transaction(std::uint8_t u1, std::uint8_t u2, const std::uint8_t* user, const std::uint8_t* amount, size_t amt);
};

class User {
	std::wstring name;
	static int num;
	std::size_t ecexuted;
	std::vector<Event> events;
	std::vector<Event::STATE> states;
public:
	const uint8_t id;
	User(wchar_t *name);
	const std::wstring& getName() const;
	void addEvent(std::uint8_t target, std::uint8_t picId, std::uint8_t id, bool picLoaded);
	void updatePic(std::uint8_t picId);
	void loadPic(std::uint8_t picId);
};