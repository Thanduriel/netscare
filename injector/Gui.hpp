#pragma once

#include <windows.h>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"

struct Address {
	std::wstring name;
	int userId;
	int tickets;
	Address() : name{ L"" }, userId{ -1 }, tickets{ 0 } {}
	Address(std::wstring&& name, int userId, int tickets) : name{ name }, userId{ userId }, tickets{ tickets } {}
};

constexpr int MAX_USERNAMELENGTH = 32;

constexpr int WM_DESTROYCHILD = WM_USER + 2; // child has been detsroyd, lParam = ChlidHandle
constexpr int WM_NOTIFICATIONCALLBACK = WM_USER + 3;
constexpr int WM_SETUPEVENT = WM_USER + 4; // lParam *ScareEvent
constexpr int WM_UPDATEVENT = WM_USER + 5; // wParam int eventId
constexpr int WM_TRIGGEREVENT = WM_USER + 6; // wParam int event_id
constexpr int WM_REFRESH = WM_USER + 7;
constexpr int WM_LOGIN = WM_USER + 8; // lParam const wchar_t ptr

constexpr int COM_ADDQUEUE = 2;
constexpr int COM_CLOSE = 3;
constexpr int COM_ADDEVENT = 4;
constexpr int COM_SETFILE = 5;
constexpr int COM_SETTARGET = 6;
constexpr int COM_SHOW = 7;
constexpr int COM_READKEY = 8;
constexpr int COM_EXECUTE_EVENT = 9;
constexpr int COM_SELECTITEM = 10; // lParam = item id;
constexpr int COM_LOGIN = 11;

struct Dim {
	constexpr Dim(int x, int y, int width, int height) : x{ x }, y{ y }, width{ width }, height{ height }, boundW{ x + width }, boundH{y + height} {}
	const int x, y, width, height, boundW, boundH;
};

constexpr Dim DIM_ADDRESSLINE{ 10, 5, 300, 50 };
constexpr Dim DIM_ADDRESSBOOK{ 200, 200, DIM_ADDRESSLINE.width + 50, 400 };

constexpr Dim DIM_MAINWIN{ 100, 100, 500, 500 };
constexpr Dim DIM_QUEUEBOX{ 10, 110, 450, 300 };
constexpr Dim DIM_QUEUEWIN{ 5, 80, 80, DIM_QUEUEBOX.height - 100};
constexpr Dim DIM_QUEUACTION{ 0, 2, DIM_QUEUEWIN.width, DIM_QUEUEWIN.y - 4 };
constexpr int QUEUE_WIN_PRO_BOX = DIM_QUEUEBOX.width / DIM_QUEUEWIN.boundW;

constexpr Dim DIM_EVENTWIN{ 2, 5, DIM_QUEUEWIN.width - 24, 50 };
constexpr int QUEUE_EVENT_PRO_WIN = DIM_QUEUEWIN.height / DIM_EVENTWIN.boundH;

constexpr int NIF_ID = 1;

LRESULT CALLBACK AddressBookProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MainWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueBoxProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueActionProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EventWinPro(HWND, UINT, WPARAM, LPARAM);

bool getOpenFileName(char* fileName, int size, const char* = NULL);

struct AddressBookState {
	AddressBookState(std::vector<Address>& adr, std::vector<int>& res) : addresses{ adr }, reservt{ res } {}
	std::vector<Address>& addresses;
	std::vector<int>& reservt;
	int scroll;
	int linePerBook;
	HWND parent;
};

struct MainWinState {
	HWND hUsername{ 0 }, hQueueBox{ 0 }, hMain, hLoginButton{ 0 };
	wchar_t username[MAX_USERNAMELENGTH];
	HMENU nifMenu;
	MainWinState(std::vector<Address>& adrs, std::vector<int>& res) : addresses{ adrs }, reservt{ res } {}
	std::vector<Address>& addresses;
	std::vector<int>& reservt;
};

struct QueueBoxState {
	int barHeight{ 5 };
	int scroll{ 0 };
	struct Queue {
		Queue() : hActionBox{ 0 }, hQueue{ 0 } {}
		Queue(HWND hQueue, HWND hActionBox) : hQueue{ hQueue }, hActionBox{ hActionBox } {}
		HWND hQueue, hActionBox;
	};
	std::vector<Queue> hQueues;
};

struct QueueActionState {
	int keyCode;
	HWND hKey;
	bool readKey;
};

struct QueueWinState {
	int scroll{ 0 };
	int keyCode{ 0 };
	std::vector<HWND> hEvents;
};

class ScareEventCp;

class ScareEvent {
	static std::size_t num;
	ScareEventCp * cp;
	void(ScareEventCp::*cl)();
	bool hasCp;
	HWND handle{ NULL };
public:
	enum STATE { NOT, WILLSET, SETTET, EXECUTED } evState{ NOT };
	void setHandle(HWND hwnd) {
		handle = hwnd;
	}
	void changeState(STATE state) {
		if (state != evState) {
			evState = state;
			if (handle) PostMessageW(handle, WM_REFRESH, NULL, NULL);
		}
	}
	bool hasNoCopy() { return !hasCp; }
	void unRef() {
		if (hasCp) cp = nullptr;
	}
	bool setCopy(ScareEventCp *cse, void (ScareEventCp::*close)()) {
		if (hasCp) return false;
		hasCp = true;
		cp = cse;
		cl = close;
		return true;
	}
	ScareEvent() : id{ num++ }, cp{ nullptr }, cl{ nullptr }, hasCp{false} {}
	ScareEvent(const ScareEvent&) = delete;
	ScareEvent(const ScareEvent&&) = delete;
	ScareEvent& operator=(const ScareEvent&) = delete;
	ScareEvent& operator=(const ScareEvent&&) = delete;
	~ScareEvent() {
		if (hasCp && cp) (cp->*cl)();
		if (file) delete[] file;
	}
	const std::size_t id;
	char *file{ nullptr };
	int target{ -1 };
};

class ScareEventCp {
	bool valid;
	ScareEvent* ch;
public:
	const ScareEvent::STATE& evState;
	const std::size_t id;
	char* const& file;
	const int & target;
	void changeState(ScareEvent::STATE state) {
		if(valid) ch->changeState(state);
	}
	void close() { 
		if (valid) ch->unRef();
		valid = false;
	}
	bool isValid() { return valid; }
	ScareEventCp(ScareEvent* se) : ch{ se }, evState{ se->evState }, id{ se->id }, file{ se->file }, target{se->target} {
		valid = se->setCopy(this, &ScareEventCp::close);
	}
	~ScareEventCp() {
		if(valid) ch->unRef();
	}
};

struct EventWinState : public ScareEvent {
	bool readTarget;
	HWND hState;
	MainWinState* mainState;
};
// setup Event data = PtrScareEvent
// cancle Event data = int<id>
// trigger Event data = int<id>
// login data = wchar<username>
struct Action {
	enum TYPE { EV_SETUP, EV_TRIGGER, EV_UPADTE, SETCOLOR, CLOSE, NOTHING, LOGIN } type;
	unsigned char *data;
	const std::size_t size;
	bool clear{ true };
	Action(TYPE t, const std::size_t size = 0, unsigned char* data = nullptr, bool clear = true) : type{ t }, data{ data }, size{ size }, clear{clear} {}
	~Action() {
		if (clear && data) delete[] data;
	}
	unsigned char* begin() { return data; }
	unsigned char* end() { return data + size; }
};

class Gui {
	HWND _hInput;
	HCURSOR _hCurser;
	HICON _hIcon;
	HMENU _iconMenu;
	std::vector<Address>& addresses;
	std::vector<int> reservt; // tickets on hold
	unsigned char bgColor[4];
public:
	Gui(HINSTANCE, std::vector<Address>&);
	~Gui() {
		DestroyIcon(_hIcon);
		DestroyCursor(_hCurser);
	}
	const Action update();
	void SetUserName(const wchar_t* username);
};

constexpr wchar_t applicationName[] = L"NetScare";

constexpr BYTE andMask[] = { 0xFF, 0xFF, 0xFF, 0xFF,   // line 1 
0xFF, 0xFF, 0xC3, 0xFF,   // line 2 
0xFF, 0xFF, 0x00, 0xFF,   // line 3 
0xFF, 0xFE, 0x00, 0x7F,   // line 4 

0xFF, 0xFC, 0x00, 0x1F,   // line 5 
0xFF, 0xF8, 0x00, 0x0F,   // line 6 
0xFF, 0xF8, 0x00, 0x0F,   // line 7 
0xFF, 0xF0, 0x00, 0x07,   // line 8 

0xFF, 0xF0, 0x00, 0x03,   // line 9 
0xFF, 0xE0, 0x00, 0x03,   // line 10 
0xFF, 0xE0, 0x00, 0x01,   // line 11 
0xFF, 0xE0, 0x00, 0x01,   // line 12 

0xFF, 0xF0, 0x00, 0x01,   // line 13 
0xFF, 0xF0, 0x00, 0x00,   // line 14 
0xFF, 0xF8, 0x00, 0x00,   // line 15 
0xFF, 0xFC, 0x00, 0x00,   // line 16 

0xFF, 0xFF, 0x00, 0x00,   // line 17 
0xFF, 0xFF, 0x80, 0x00,   // line 18 
0xFF, 0xFF, 0xE0, 0x00,   // line 19 
0xFF, 0xFF, 0xE0, 0x01,   // line 20 

0xFF, 0xFF, 0xF0, 0x01,   // line 21 
0xFF, 0xFF, 0xF0, 0x01,   // line 22 
0xFF, 0xFF, 0xF0, 0x03,   // line 23 
0xFF, 0xFF, 0xE0, 0x03,   // line 24 

0xFF, 0xFF, 0xE0, 0x07,   // line 25 
0xFF, 0xFF, 0xC0, 0x0F,   // line 26 
0xFF, 0xFF, 0xC0, 0x0F,   // line 27 
0xFF, 0xFF, 0x80, 0x1F,   // line 28 

0xFF, 0xFF, 0x00, 0x7F,   // line 29 
0xFF, 0xFC, 0x00, 0xFF,   // line 30 
0xFF, 0xF8, 0x03, 0xFF,   // line 31 
0xFF, 0xFC, 0x3F, 0xFF };  // line 32 

constexpr BYTE xorMap[] = { 0x00, 0x00, 0x00, 0x00,   // line 1 
0x00, 0x00, 0x00, 0x00,   // line 2 
0x00, 0x00, 0x00, 0x00,   // line 3 
0x00, 0x00, 0x00, 0x00,   // line 4 

0x00, 0x00, 0x00, 0x00,   // line 5 
0x00, 0x00, 0x00, 0x00,   // line 6 
0x00, 0x00, 0x00, 0x00,   // line 7 
0x00, 0x00, 0x38, 0x00,   // line 8 

0x00, 0x00, 0x7C, 0x00,   // line 9 
0x00, 0x00, 0x7C, 0x00,   // line 10 
0x00, 0x00, 0x7C, 0x00,   // line 11 
0x00, 0x00, 0x38, 0x00,   // line 12 

0x00, 0x00, 0x00, 0x00,   // line 13 
0x00, 0x00, 0x00, 0x00,   // line 14 
0x00, 0x00, 0x00, 0x00,   // line 15 
0x00, 0x00, 0x00, 0x00,   // line 16 

0x00, 0x00, 0x00, 0x00,   // line 17 
0x00, 0x00, 0x00, 0x00,   // line 18 
0x00, 0x00, 0x00, 0x00,   // line 19 
0x00, 0x00, 0x00, 0x00,   // line 20 

0x00, 0x00, 0x00, 0x00,   // line 21 
0x00, 0x00, 0x00, 0x00,   // line 22 
0x00, 0x00, 0x00, 0x00,   // line 23 
0x00, 0x00, 0x00, 0x00,   // line 24 

0x00, 0x00, 0x00, 0x00,   // line 25 
0x00, 0x00, 0x00, 0x00,   // line 26 
0x00, 0x00, 0x00, 0x00,   // line 27 
0x00, 0x00, 0x00, 0x00,   // line 28 

0x00, 0x00, 0x00, 0x00,   // line 29 
0x00, 0x00, 0x00, 0x00,   // line 30 
0x00, 0x00, 0x00, 0x00,   // line 31 
0x00, 0x00, 0x00, 0x00 };  // line 32 