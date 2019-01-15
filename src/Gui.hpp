#include <windows.h>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"
constexpr int WM_SETCOLOR = WM_USER + 1; // wParam = rgba8
constexpr int WM_DESTROYCHILD = WM_USER + 2; // child has been detsroyd, lParam = ChlidHandle

constexpr int COM_SETCOLOR = 1;
constexpr int COM_ADDQUEUE = 2;
constexpr int COM_CLOSE = 3;
constexpr int COM_ADDEVENT = 4;
constexpr int COM_SETFILE = 5;
constexpr int COM_SETTARGET = 6;

struct Dim {
	constexpr Dim(int x, int y, int width, int height) : x{ x }, y{ y }, width{ width }, height{ height }, boundW{ x + width }, boundH{y + height} {}
	const int x, y, width, height, boundW, boundH;
};

constexpr Dim DIM_MAINWIN{ 100, 100, 500, 500 };
constexpr Dim DIM_QUEUEBOX{ 10, 110, 450, 300 };
constexpr Dim DIM_QUEUEWIN{ 5, 25, 80, DIM_QUEUEBOX.height - 30};
constexpr int QUEUE_WIN_PRO_BOX = DIM_QUEUEBOX.width / DIM_QUEUEWIN.boundW;

constexpr Dim DIM_EVENTWIN{ 2, 5, DIM_QUEUEWIN.width - 5, 30 };
constexpr int QUEUE_EVENT_PRO_WIN = DIM_QUEUEWIN.height / DIM_EVENTWIN.boundH;

LRESULT CALLBACK MainWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueBoxProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EventWinPro(HWND, UINT, WPARAM, LPARAM);

bool getOpenFileName(char* fileName, int size, const char* = NULL);

struct MainWinState {
	HWND hColorE{ 0 }, hQueueBox{ 0 };
};

struct QueueBoxState {
	int barHeight{ 5 };
	int scroll{ 0 };
	struct Queue {
		Queue() : hAddButton{ 0 }, hQueue{ 0 } {}
		Queue(HWND hQueue, HWND hAddButton) : hQueue{ hQueue }, hAddButton{ hAddButton } {}
		HWND hQueue, hAddButton;
	};
	std::vector<Queue> hQueues;
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
public:
	Gui(HINSTANCE);
	const Action update();
};