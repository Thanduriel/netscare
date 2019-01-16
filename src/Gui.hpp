#include <windows.h>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"
constexpr int WM_SETCOLOR = WM_USER + 1; // wParam = rgba8
constexpr int WM_DESTROYCHILD = WM_USER + 2; // child has been detsroyd, lParam = ChlidHandle
constexpr int WM_NOTIFICATIONCALLBACK = WM_USER + 3;
constexpr int WM_REDIRECTED_KEYDOWN = WM_USER + 4;

constexpr int COM_SETCOLOR = 1;
constexpr int COM_ADDQUEUE = 2;
constexpr int COM_CLOSE = 3;
constexpr int COM_ADDEVENT = 4;
constexpr int COM_SETFILE = 5;
constexpr int COM_SETTARGET = 6;
constexpr int COM_SHOW = 7;
constexpr int COM_READKEY = 8;
constexpr int COM_EXECUTE_EVENT = 9;

struct Dim {
	constexpr Dim(int x, int y, int width, int height) : x{ x }, y{ y }, width{ width }, height{ height }, boundW{ x + width }, boundH{y + height} {}
	const int x, y, width, height, boundW, boundH;
};

constexpr Dim DIM_MAINWIN{ 100, 100, 500, 500 };
constexpr Dim DIM_QUEUEBOX{ 10, 110, 450, 300 };
constexpr Dim DIM_QUEUEWIN{ 5, 80, 80, DIM_QUEUEBOX.height - 82};
constexpr Dim DIM_QUEUACTION{ 0, 2, DIM_QUEUEWIN.width, DIM_QUEUEWIN.y - 4 };
constexpr int QUEUE_WIN_PRO_BOX = DIM_QUEUEBOX.width / DIM_QUEUEWIN.boundW;

constexpr Dim DIM_EVENTWIN{ 2, 5, DIM_QUEUEWIN.width - 24, 30 };
constexpr int QUEUE_EVENT_PRO_WIN = DIM_QUEUEWIN.height / DIM_EVENTWIN.boundH;

constexpr int NIF_ID = 1;

LRESULT CALLBACK MainWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueBoxProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueActionProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EventWinPro(HWND, UINT, WPARAM, LPARAM);

bool getOpenFileName(char* fileName, int size, const char* = NULL);

struct MainWinState {
	HWND hColorE{ 0 }, hQueueBox{ 0 };
	HMENU nifMenu;
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

struct EventWinState {
	char* file{ nullptr };
	int target{ 0 };
};

struct Action {
	enum TYPE { SETCOLOR, CLOSE, NOTHING } type;
	unsigned char *data;
	const std::size_t size;
	Action(TYPE t, const std::size_t size = 0, unsigned char* data = nullptr) : type{ t }, data{ data }, size{ size } {}
	~Action() {
		if (data) delete[] data;
	}
	unsigned char* begin() { return data; }
	unsigned char* end() { return data + size; }
};

class Gui {
	HWND _hInput;
	HCURSOR _hCurser;
	HICON _hIcon;
	HMENU _iconMenu;
public:
	Gui(HINSTANCE);
	~Gui() {
		DestroyIcon(_hIcon);
		DestroyCursor(_hCurser);
	}
	const Action update();
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