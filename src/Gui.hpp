#include <windows.h>

#include "../src/interface.hpp"
#include "../src/PipeServer.hpp"
constexpr int WM_SETCOLOR = WM_USER + 1; // wParam = rgba8
constexpr int WM_DESTROYCHILD = WM_USER + 2; // child has been detsroyd, lParam = ChlidHandle

constexpr int COM_SETCOLOR = 1;
constexpr int COM_ADDQUEUE = 2;
constexpr int COM_CLOSE = 3;

struct Dim {
	constexpr Dim(int x, int y, int width, int height) : x{ x }, y{ y }, width{ width }, height{ height }, boundW{ x + width }, boundH{y + height} {}
	const int x, y, width, height, boundW, boundH;
};

constexpr Dim DIM_QUEUEBOX{ 10, 110, 450, 300 };
constexpr Dim DIM_QUEUEWIN{ 5, 5, 50, DIM_QUEUEBOX.height - 10};
constexpr int QUEUE_WIN_PRO_BOX = DIM_QUEUEBOX.width / (DIM_QUEUEWIN.boundW);

LRESULT CALLBACK MainWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueBoxProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK QueueWinProc(HWND, UINT, WPARAM, LPARAM);



struct MainWinState {
	HWND hColorE{ 0 }, hQueueBox{ 0 };
};

struct QueueBoxState {
	int barHeight{ 5 };
	int scroll{ 0 };
	std::vector<HWND> hQueues;
};

struct QueueWinState {
	int scroll{ 0 };
	int keyCode{ 0 };
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