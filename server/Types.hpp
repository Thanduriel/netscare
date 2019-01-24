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
	void Realloc(int size);
	void AddUser();
	const std::uint8_t* GetTickets(std::uint8_t userId) const;
	std::uint8_t GetTicket(std::uint8_t userId, std::uint8_t targetId) const;
	enum TRASNACTION { SUUCCESS, NOT_ENOUGH, TOO_MANY };
	TRASNACTION Transaction(std::uint8_t u1, std::uint8_t u2, const std::uint8_t* user, const std::uint8_t* amount, size_t amt);
};

class User {
	std::wstring name;
	static int num;
	std::size_t ecexuted;
	std::vector<Event> events;
	std::vector<Event::STATE> states;
public:
	const uint8_t id;
	User(const wchar_t *name);
	User(const char* name);
	const std::wstring& GetName() const;
	void AddEvent(std::uint8_t target, std::uint8_t picId, std::uint8_t id, bool picLoaded);
	void UpdatePic(std::uint8_t picId);
	void LoadPic(std::uint8_t picId);
};