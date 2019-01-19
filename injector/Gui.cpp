#include "Gui.hpp"

std::size_t ScareEvent::num = 0;

static void showError(const char* caption, DWORD error) {
	LPSTR msg = 0;
	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		0,
		(LPSTR)&msg, // muss anscheinend so, steht in der Doku
		0,
		NULL) == 0) {
		MessageBox(NULL, "Cant parse Error", caption, MB_OK | MB_ICONERROR);
		return;
	}
	MessageBox(NULL, msg, caption, MB_OK | MB_ICONERROR);
	if (msg) {
		LocalFree(msg);
		msg = 0;
	}
}

struct Icon {
	int width, height;
	BYTE planes, bitsPixel;
	const BYTE *andBits, *xorBits;
	constexpr Icon(int width, int height, BYTE planes, BYTE bitsPixel, const BYTE* andBits, const BYTE* xorBits)
		: width{ width }, height{ height }, planes{ planes }, bitsPixel{ bitsPixel }, andBits{ andBits }, xorBits{xorBits} {}
	HICON create(HINSTANCE hInst) const {
		return CreateIcon(hInst, width, height, planes, bitsPixel, andBits, xorBits);
	}
};
constexpr Icon icon{32, 32, 1, 1, andMask, xorMap};

Gui::Gui(HINSTANCE hInst, std::vector<Address>& addresses) : addresses{addresses} {
	reservt.resize(addresses.size());
	memset(reservt.data(), 0, reservt.size() * sizeof(int));
	_hCurser = LoadCursor(NULL, IDC_ARROW);
	_hIcon = icon.create(hInst);
	_iconMenu = CreatePopupMenu();
	AppendMenuW(_iconMenu, MF_STRING, COM_SHOW, L"Open");
	AppendMenuW(_iconMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(_iconMenu, MF_STRING, COM_CLOSE, L"Quit");

	WNDCLASSW winC = { 0 };
	winC.hCursor = _hCurser;
	winC.hIcon = _hIcon;
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"MainWindow";
	winC.lpfnWndProc = MainWinProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	winC = { 0 };
	winC.hCursor = _hCurser;
	winC.hIcon = _hIcon;
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"AddressBookWin";
	winC.lpfnWndProc = AddressBookProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	winC = {0};
	winC.hCursor = _hCurser;
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"QueueBoxWin";
	winC.lpfnWndProc = QueueBoxProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	winC = { 0 };
	winC.hCursor = _hCurser;
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"QueueWinWin";
	winC.lpfnWndProc = QueueWinProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	winC = { 0 };
	winC.hCursor = _hCurser;
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"EventWinWin";
	winC.lpfnWndProc = EventWinPro;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	winC = { 0 };
	winC.hCursor = _hCurser;
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"QueueActionWin";
	winC.lpfnWndProc = QueueActionProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	{
		MainWinState *pState = new (std::nothrow) MainWinState{addresses, reservt};
		pState->nifMenu = _iconMenu;
		_hInput = CreateWindowW(L"MainWindow", applicationName, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			DIM_MAINWIN.x, DIM_MAINWIN.y, DIM_MAINWIN.width, DIM_MAINWIN.height, NULL, NULL, NULL, pState);
		pState->hMain = _hInput;
	}

	NOTIFYICONDATAW note = { 0 };
	note.cbSize = sizeof(NOTIFYICONDATAW);
	note.hWnd = _hInput;
	note.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	note.uCallbackMessage = WM_NOTIFICATIONCALLBACK;
	note.hIcon = _hIcon;
	note.uID = NIF_ID;
	wcscpy(note.szTip, applicationName);
	note.dwState = NULL;
	note.dwStateMask = NULL;
	note.uVersion = NOTIFYICON_VERSION_4;
	if (!Shell_NotifyIconW(NIM_ADD, &note)) {
		showError("Can't Craete NotifyIcon", GetLastError());
	}
}

const Action Gui::update() {
	MSG winMsg = { 0 };
	if (GetMessage(&winMsg, NULL, NULL, NULL)) {
		TranslateMessage(&winMsg);
		switch (winMsg.message) {
		case WM_KEYDOWN:
			switch (winMsg.wParam) {
			case VK_RETURN:
				SendMessageW(_hInput, winMsg.message, winMsg.wParam, (LPARAM)winMsg.hwnd);
				break;
			default:
				DispatchMessageW(&winMsg);
			}
			break;
		case WM_SETUPEVENT: {
			unsigned char *wp = new unsigned char[sizeof(ScareEvent*)];
			*reinterpret_cast<LPARAM*>(wp) = winMsg.lParam;
			return Action(Action::EV_SETUP, sizeof(ScareEvent*), wp);
		} break;
		case WM_UPDATEVENT: {
			unsigned char *wp = new unsigned char[sizeof(int)];
			*reinterpret_cast<int*>(wp) = winMsg.wParam;
			return Action(Action::EV_UPADTE, sizeof(int), wp);
		}	break;
		case WM_TRIGGEREVENT: {
			unsigned char *wp = new unsigned char[sizeof(int)];
			*reinterpret_cast<int*>(wp) = winMsg.wParam;
			return Action(Action::EV_TRIGGER, sizeof(int), wp);
		} break;
		case WM_SETCOLOR: {
			unsigned char *wp = new unsigned char[4];
			*reinterpret_cast<WPARAM*>(wp) = winMsg.wParam;
			return Action(Action::SETCOLOR, 4, wp);
		}
		break;
		default:
			DispatchMessageW(&winMsg);
		}
		return Action(Action::NOTHING);
	} else {
		return Action(Action::CLOSE);
	}
}

void addUI(HWND hWnd, AddressBookState *pState) {
	int h = 0;
	int num = 1;
	for (std::size_t i = 0; i < pState->addresses.size(); ++i) {
		Address adr = pState->addresses[i];
		CreateWindowW(L"button", (adr.name + L": Tickets " + std::to_wstring(adr.tickets) + L" (" + std::to_wstring(pState->reservt[i]) + L")").c_str(), 
			WS_CHILD | WS_BORDER |WS_VISIBLE | ( adr.tickets - pState->reservt[i] > 0 ? 0 : WS_DISABLED),
			DIM_ADDRESSLINE.x, DIM_ADDRESSLINE.y + h, DIM_ADDRESSLINE.width, DIM_ADDRESSLINE.height, hWnd, (HMENU)num, NULL, NULL);
		h += DIM_ADDRESSLINE.boundH;
		++num;
	}
	CreateWindowA("button", "Self",
		WS_CHILD | WS_BORDER | WS_VISIBLE,
		DIM_ADDRESSLINE.x, DIM_ADDRESSLINE.y + h, DIM_ADDRESSLINE.width, DIM_ADDRESSLINE.height, hWnd, (HMENU)num, NULL, NULL);
	h += DIM_ADDRESSLINE.boundH;
	RECT rect;
	GetClientRect(hWnd, &rect);
	int wH = rect.bottom - rect.top;
	pState->linePerBook = DIM_ADDRESSLINE.boundH / wH;
	int max = h - wH;
	if (max < 0) max = 0;
	SetScrollRange(hWnd, SB_VERT, 0,max, true);
}

void addUI(HWND hWnd, MainWinState* pState) {
	CreateWindowW(L"static", L"Color(r,g,b): ", WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, hWnd, NULL, NULL, NULL);
	pState->hColorE = CreateWindowW(L"edit", L"255,255,255", WS_VISIBLE | WS_CHILD, 110, 10, 100, 20, hWnd, NULL, NULL, NULL);
	CreateWindowW(L"button", L"set bg color", WS_VISIBLE | WS_CHILD, 10, 60, 100, 20, hWnd, (HMENU)COM_SETCOLOR, NULL, NULL);
	CreateWindowW(L"button", L"AddQueue", WS_VISIBLE | WS_CHILD, 110, 60, 100, 20, hWnd, (HMENU)COM_ADDQUEUE, NULL, NULL);

	QueueBoxState *qbs = new (std::nothrow) QueueBoxState{};
	pState->hQueueBox = CreateWindowW(L"QueueBoxWin", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_HSCROLL ,
		DIM_QUEUEBOX.x, DIM_QUEUEBOX.y, DIM_QUEUEBOX.width, DIM_QUEUEBOX.height,
		hWnd, NULL, NULL, qbs);
}

void addUI(HWND hWnd, QueueWinState* pState) {}

void addUI(HWND hWnd, QueueActionState* pState) {
	RECT rect;
	GetClientRect(hWnd, &rect);
	int w = DIM_QUEUACTION.width,
		h = DIM_QUEUACTION.height;
	h = (h - 8) / 3;
	CreateWindowW(L"button", L"Add Event", WS_VISIBLE | WS_CHILD, 0, 2, w, h, hWnd, (HMENU)COM_ADDEVENT, NULL, NULL);
	CreateWindowW(L"button", L"Delete", WS_VISIBLE | WS_CHILD, 0, 4 + h, w, h, hWnd, (HMENU)COM_CLOSE, NULL, NULL);
	pState->hKey = CreateWindowW(L"button", L"Key", WS_VISIBLE | WS_CHILD, 0, 6 + 2 * h, w, h, hWnd, (HMENU)COM_READKEY, NULL, NULL);
	pState->readKey = false;
}

void addUI(HWND hWnd, EventWinState* pState) {
	RECT rect;
	GetClientRect(hWnd, &rect);
	int w = rect.right - rect.left,
		h = rect.bottom - rect.top;
	w = (w - 6) / 2;
	h = (h -6) / 2;
	CreateWindowW(L"button", L"X", WS_VISIBLE | WS_CHILD, 2, 2, w, h, hWnd, (HMENU)COM_CLOSE, NULL, NULL);
	pState->hState = CreateWindowW(L"static", L"U", WS_VISIBLE | WS_CHILD, 4 + w, 2, w, h, hWnd, NULL, NULL, NULL);
	CreateWindowW(L"button", L"F", WS_VISIBLE | WS_CHILD, 2, h + 2, w, h, hWnd, (HMENU)COM_SETFILE, NULL, NULL);
	CreateWindowW(L"button", L"T", WS_VISIBLE | WS_CHILD, 4 + w, h + 2, w , h, hWnd, (HMENU)COM_SETTARGET, NULL, NULL);
}

LRESULT CALLBACK MainWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	MainWinState *pState = reinterpret_cast<MainWinState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_SHOWWINDOW:
		break;
	case WM_CREATE: {
		CREATESTRUCTW *pCreat = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<MainWinState*>(pCreat->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);
		addUI(hWnd, pState);
	}	break;
	case WM_COMMAND:
		com = wParam;
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			if (lParam == (LPARAM)pState->hColorE) com = COM_SETCOLOR;
			break;
		}
		break;
	case WM_CLOSE:
		if (!ShowWindow(hWnd, SW_HIDE))
			showError("Can't Hide Win", GetLastError());
		break;
	case WM_DESTROY: {
		delete pState;
		NOTIFYICONDATAA note = { 0 };
		note.uID = NIF_ID;
		note.hWnd = hWnd;
		Shell_NotifyIconA(NIM_DELETE, &note);
		PostQuitMessage(0);
		break;
	}
	case WM_NOTIFICATIONCALLBACK: {
		switch (LOWORD(lParam)) {
		case WM_CONTEXTMENU:
		case WM_RBUTTONDOWN: {
			POINT posC;
			GetCursorPos(&posC);
			if (IsWindowVisible(hWnd))
				EnableMenuItem(pState->nifMenu, 0, MF_BYPOSITION | MF_GRAYED);
			else
				EnableMenuItem(pState->nifMenu, 0, MF_BYPOSITION | MF_ENABLED);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(pState->nifMenu, TPM_LEFTBUTTON | TPM_RIGHTALIGN | TPM_BOTTOMALIGN, posC.x, posC.y, 0, hWnd, NULL);
		}	break;
		case NIN_KEYSELECT:
		case NIN_SELECT:
		case WM_LBUTTONDOWN: com = COM_SHOW; break;
		}
	}	break;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	switch (com) {
	case COM_SETCOLOR: {
		wchar_t text[32];
		GetWindowTextW(pState->hColorE, text, 32);
		unsigned char rgba[4] = { 0, 0, 0, 0 };
		rgba[3] = 255;
		swscanf_s(text, L"%hhu, %hhu, %hhu", rgba, rgba + 1, rgba + 2);
		PostMessageW(NULL, WM_SETCOLOR, *reinterpret_cast<WPARAM*>(rgba), NULL);
	} break;
	case COM_ADDQUEUE:
		PostMessageW(pState->hQueueBox, WM_COMMAND, COM_ADDQUEUE, lParam);
		break;
	case COM_SHOW: 
		if (!IsWindowVisible(hWnd))ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		break;
	case COM_CLOSE: DestroyWindow(hWnd); break;
	// default: MessageBox(NULL, std::to_string(com).c_str(), "COMANND", MB_OK);
	}
	return 0;
}

LRESULT CALLBACK QueueBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	QueueBoxState *pState = reinterpret_cast<QueueBoxState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_CREATE: {
		CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<QueueBoxState*>(pCreate->lpCreateParams);
		pState->scroll = 0;
		pState->hQueues.resize(0);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);
		SetScrollRange(hWnd, SB_HORZ, 0, 0, true);
	}	break;
	case WM_HSCROLL: {
		WORD low = LOWORD(wParam), high = HIWORD(wParam);
		int diff = 0;
		bool redraw = true;
		switch (low)
		{
		case SB_THUMBTRACK: diff = high - pState->scroll; redraw = false; break;
		case SB_THUMBPOSITION: diff = high - pState->scroll; break;
		case SB_LEFT: diff = -pState->scroll; break;
		case SB_RIGHT: diff = pState->hQueues.size() * DIM_QUEUEWIN.boundW; break;
		case SB_LINELEFT: diff = -DIM_QUEUEWIN.boundW; break;
		case SB_LINERIGHT: diff = DIM_QUEUEWIN.boundW; break;
		case SB_PAGELEFT: diff = QUEUE_WIN_PRO_BOX * DIM_QUEUEWIN.boundW; break;
		case SB_PAGERIGHT: diff = QUEUE_WIN_PRO_BOX * DIM_QUEUEWIN.boundW; break;
		}
		if (diff != 0) {
			int maxD = (pState->hQueues.size() - QUEUE_WIN_PRO_BOX) * DIM_QUEUEWIN.boundW - pState->scroll,
				minD = -pState->scroll;
			if (diff > maxD) diff = maxD;
			if (diff < minD) diff = minD;
			if (diff != 0) {
				pState->scroll += diff;
				SetScrollPos(hWnd, SB_HORZ, pState->scroll, redraw);
				ScrollWindow(hWnd, -diff, 0, NULL, NULL);
			}
		}
	}	break;
	case WM_DESTROYCHILD: {
		std::size_t num = 0;
		HWND delet = (HWND)lParam;
		RECT self;
		GetWindowRect(hWnd, &self);
		for (std::size_t i = 0; i < pState->hQueues.size(); ++i) {
			if (pState->hQueues[i].hActionBox == delet) {
				DestroyWindow(pState->hQueues[i].hQueue);
				continue;
			}
			if (num < i) {
				RECT rect;
				GetWindowRect(pState->hQueues[i].hQueue, &rect);
				int w = rect.right - rect.left,
					h = rect.bottom - rect.top;
				rect.top -= self.top + 1;
				rect.left -= self.left + (i - num) * DIM_QUEUEWIN.boundW + 1;
				MoveWindow(pState->hQueues[i].hQueue, rect.left, rect.top, w, h, TRUE);
				GetWindowRect(pState->hQueues[i].hActionBox, &rect);
				w = rect.right - rect.left;
				h = rect.bottom - rect.top;
				rect.top -= self.top + 1;
				rect.left -= self.left + (i - num) * DIM_QUEUEWIN.boundW + 1;
				MoveWindow(pState->hQueues[i].hActionBox, rect.left, rect.top, w, h, TRUE);
				pState->hQueues[num] = pState->hQueues[i];
			}
			++num;
		}
		pState->hQueues.resize(num);
		int max = (num - QUEUE_WIN_PRO_BOX) * DIM_QUEUEWIN.boundW;
		if (max < 0) max = 0;
		SetScrollRange(hWnd, SB_HORZ, 0, max, true);
		if (pState->scroll > max) {
			ScrollWindow(hWnd, pState->scroll - max, 0, NULL, NULL);
			pState->scroll = max;
		}
	} break;
	case WM_DESTROY:
		if(pState) delete pState;
		break;
	case WM_COMMAND:
		com = wParam;
		break;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	switch (com) {
	case COM_ADDEVENT: {
		for (const QueueBoxState::Queue& q : pState->hQueues) {
			if (q.hActionBox == (HWND)lParam) {
				PostMessageW(q.hQueue, WM_COMMAND, wParam, lParam);
				break;
			}
		}
	} break;
	case COM_EXECUTE_EVENT: {
		for (const QueueBoxState::Queue& q : pState->hQueues) {
			if (q.hActionBox == (HWND)lParam) {
				PostMessageW(q.hQueue, WM_COMMAND, wParam, lParam);
				break;
			}
		}
	}	break;
	case COM_ADDQUEUE: {
		static int num = 0;
		QueueWinState * state = new (std::nothrow) QueueWinState;
		state->keyCode = num;
		QueueActionState* actionState = new (std::nothrow) QueueActionState;
		++num;
		int x = DIM_QUEUEWIN.x + DIM_QUEUEWIN.boundW * pState->hQueues.size() - pState->scroll;
		pState->hQueues.push_back(QueueBoxState::Queue(
			CreateWindowW(L"QueueWinWin", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
			x, DIM_QUEUEWIN.y, DIM_QUEUEWIN.width, DIM_QUEUEWIN.height,
			hWnd, NULL, NULL, state),
			CreateWindowW(L"QueueActionWin", L"", WS_VISIBLE | WS_CHILD, x, DIM_QUEUACTION.y, DIM_QUEUACTION.width, DIM_QUEUACTION.height, hWnd, NULL, NULL, actionState)));
		if (!pState->hQueues.back().hQueue || !pState->hQueues.back().hActionBox)
			showError("PQueueError", GetLastError());
		if (pState->hQueues.size() > QUEUE_WIN_PRO_BOX)
			SetScrollRange(hWnd, SB_HORZ, 0, (pState->hQueues.size() - QUEUE_WIN_PRO_BOX) * DIM_QUEUEWIN.boundW, true);
	}	break;
	// default: MessageBox(NULL, std::to_string(com).c_str(), "COMANND", MB_OK);
	}

	return 0;
}

LRESULT CALLBACK QueueWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	QueueWinState* pState = reinterpret_cast<QueueWinState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_DESTROY:
		if (pState) delete pState;
		break;
	case WM_DESTROYCHILD: {
		std::size_t num = 0;
		HWND delet = (HWND)lParam;
		RECT self;
		GetWindowRect(hWnd, &self);
		for (std::size_t i = 0; i < pState->hEvents.size(); ++i) {
			if (pState->hEvents[i] == delet) continue;
			if (num < i) {
				RECT rect;
				GetWindowRect(pState->hEvents[i], &rect);
				int w = rect.right - rect.left,
					h = rect.bottom - rect.top,
					x = rect.left - self.left - 1,
					y = rect.top - self.top - 1 - (i - num) * DIM_EVENTWIN.boundH;
				MoveWindow(pState->hEvents[i], x, y, w, h, TRUE);
				pState->hEvents[num] = pState->hEvents[i];
			}
			++num;
		}
		pState->hEvents.resize(num);
		int max = (num - QUEUE_EVENT_PRO_WIN) * DIM_EVENTWIN.boundH;
		if (max < 0) max = 0;
		SetScrollRange(hWnd, SB_VERT, 0, max, TRUE);
		if (pState->scroll > max) {
			ScrollWindow(hWnd, 0, pState->scroll - max, NULL, NULL);
			pState->scroll = max;
		}
	}	break;
	case WM_VSCROLL: {
		WORD low = LOWORD(wParam), high = HIWORD(wParam);
		int diff = 0;
		bool redraw = true;
		switch (low)
		{
		case SB_THUMBTRACK: diff = high - pState->scroll; redraw = false; break;
		case SB_THUMBPOSITION: diff = high - pState->scroll; break;
		case SB_LEFT: diff = -pState->scroll; break;
		case SB_RIGHT: diff = pState->hEvents.size() * DIM_EVENTWIN.boundH; break;
		case SB_LINELEFT: diff = -DIM_EVENTWIN.boundH; break;
		case SB_LINERIGHT: diff = DIM_EVENTWIN.boundH; break;
		case SB_PAGELEFT: diff = -QUEUE_EVENT_PRO_WIN * DIM_EVENTWIN.boundH; break;
		case SB_PAGERIGHT: diff = QUEUE_EVENT_PRO_WIN * DIM_EVENTWIN.boundH; break;
		}
		if (diff != 0) {
			int maxD = (pState->hEvents.size() - QUEUE_EVENT_PRO_WIN) * DIM_EVENTWIN.boundH - pState->scroll,
				minD = -pState->scroll;
			if (diff > maxD) diff = maxD;
			if (diff < minD) diff = minD;
			if (diff != 0) {
				pState->scroll += diff;
				SetScrollPos(hWnd, SB_VERT, pState->scroll, redraw);
				ScrollWindow(hWnd, 0, -diff, NULL, NULL);
			}
		}
	} break;
	case WM_CREATE: {
		CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<QueueWinState*>(pCreate->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);
		pState->scroll = 0;
		addUI(hWnd, pState);
		SetScrollRange(hWnd, SB_VERT, 0, 0, true);
	} break;
	case WM_COMMAND: com = wParam;
		break;
	default: return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	switch (com) {
	case COM_ADDEVENT: {
		static int num = 0;
		EventWinState * state = new (std::nothrow) EventWinState;
		++num;
		pState->hEvents.push_back(CreateWindowW(L"EventWinWin", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
			DIM_EVENTWIN.x, DIM_EVENTWIN.y + DIM_EVENTWIN.boundH * pState->hEvents.size() - pState->scroll, DIM_EVENTWIN.width, DIM_EVENTWIN.height,
			hWnd, NULL, NULL, state));
		if (!pState->hEvents.back())
			showError("PQueueError", GetLastError());
		if (pState->hEvents.size() > QUEUE_EVENT_PRO_WIN)
			SetScrollRange(hWnd, SB_VERT, 0, (pState->hEvents.size() - QUEUE_EVENT_PRO_WIN) * DIM_EVENTWIN.boundH, true);
	}	break;
	case COM_EXECUTE_EVENT: {
		if (pState->hEvents.size() == 0) break;
		HWND hEvent = (pState->hEvents[0]);
		EventWinState *eState = reinterpret_cast<EventWinState*>(GetWindowLongPtrW(hEvent, GWLP_USERDATA));
		if (eState->evState == ScareEvent::SETTET) {
			PostMessageA(NULL, WM_TRIGGEREVENT, eState->id, NULL);
		} else {
			MessageBox(NULL, "Filed is still loading", "Cant Trigger Event Now", MB_OK | MB_ICONERROR);
		}
	}  break;
	// default: MessageBox(NULL, std::to_string(com).c_str(), "COMANND", MB_OK);
	}
	return 0;
}

bool getOpenFileName(char* fileName, int size, const char* path)
{
	memset(fileName, 0, size);
	static const char initialDir[] = "Pictures";
	OPENFILENAMEA fop = { 0 };
	fop.lStructSize = sizeof(OPENFILENAMEA);
	fop.hwndOwner = NULL;
	fop.lpstrFilter = "Image\0*.PNG;*.JPEG;*.JPG\0DDS\0*.DDS\0";
	fop.lpstrCustomFilter = NULL;
	fop.nMaxCustFilter = NULL;
	fop.nFileOffset = 1;
	fop.lpstrFile = fileName;
	fop.nMaxFile = 512;
	fop.lpstrFileTitle = NULL;
	fop.lpstrInitialDir = path ? path : initialDir;
	fop.lpstrTitle = NULL;
	fop.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST;
	return GetOpenFileNameA(&fop);
}

void triggerSetupWhenComplete(HWND hWnd, EventWinState *pState) {
	if (pState->file != nullptr && pState->target >= 0) {
		if (pState->evState == ScareEvent::NOT) {
			PostMessageW(NULL, WM_SETUPEVENT, NULL, (LPARAM)pState);
			SetWindowTextW(pState->hState, L"L");
			pState->evState = ScareEvent::WILLSET;
		} else {
			PostMessageW(NULL, WM_UPDATEVENT, pState->id, NULL);
			SetWindowTextW(pState->hState, L"L");
			pState->evState = ScareEvent::WILLSET;
		}
	}
}

LRESULT CALLBACK EventWinPro(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	EventWinState* pState = reinterpret_cast<EventWinState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_DESTROYCHILD:
		EnableWindow(pState->mainState->hMain, TRUE);
		SetForegroundWindow(pState->mainState->hMain);
		break;
	case WM_CREATE: {
		CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<EventWinState*>(pCreate->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);
		pState->file = nullptr;
		pState->target = -1;
		pState->evState = ScareEvent::NOT;
		pState->readTarget = false;
		pState->mainState = nullptr;
		addUI(hWnd, pState);
	} break;
	case WM_PAINT:
		if (pState->evState == ScareEvent::WILLSET) {
			pState->evState = ScareEvent::SETTET;
			SetWindowTextW(pState->hState, L"R");
		}
		return DefWindowProcW(hWnd, msg, wParam, lParam);
		break;
	case WM_DESTROY:
		if (pState) { 
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, NULL);
			delete pState;
		};
		break;
	case WM_COMMAND: com = wParam; break;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	switch (com) {
	case COM_SELECTITEM:
		EnableWindow(pState->mainState->hMain, TRUE);
		SetForegroundWindow(pState->mainState->hMain);
		if (pState->readTarget) {
			pState->readTarget = false;
			if (pState->target == lParam) break; // no changes
			if (pState->target >= 0) pState->mainState->reservt[pState->target] -= 1;
			pState->target = lParam;
			pState->mainState->reservt[pState->target] += 1;
		}
		triggerSetupWhenComplete(hWnd, pState);
		break;
	case COM_SETFILE: {
		char name[512];
		memset(name, 0, 512);
		if (!getOpenFileName(name, 512, pState->file)) {
			showError("Can't get File", GetLastError());
		} else {
			if (pState->file) {
				if (strcmp(name, pState->file) == 0) break; // no changes
				delete[] pState->file;
			}
			char*c = name;
			while (*c) ++c;
			++c;
			pState->file = new char[c - name];
			memcpy(pState->file, name, c - name);
		}
		triggerSetupWhenComplete(hWnd, pState);
	}	break;
	case COM_SETTARGET: {
		pState->readTarget = true;
		if (!pState->mainState) {
			HWND main = hWnd, n;
			while (n = GetParent(main)) main = n;
			pState->mainState = reinterpret_cast<MainWinState*>(GetWindowLongPtrW(main, GWLP_USERDATA));
		}
		EnableWindow(pState->mainState->hMain, FALSE);
		AddressBookState* state = new (std::nothrow) AddressBookState{pState->mainState->addresses, pState->mainState->reservt};
		CreateWindowW(L"AddressBookWin", L"Your Tickets", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL,
			DIM_ADDRESSBOOK.x, DIM_ADDRESSBOOK.y, DIM_ADDRESSBOOK.width, DIM_ADDRESSBOOK.height,
			hWnd, NULL, NULL, state);
	}break;
	case COM_CLOSE: 
		PostMessageW(GetParent(hWnd), WM_DESTROYCHILD, NULL, (LPARAM)hWnd);
		DestroyWindow(hWnd);
		break;
	}
	return 0;
}

LRESULT CALLBACK QueueActionProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	QueueActionState *pState = reinterpret_cast<QueueActionState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_KILLFOCUS:
		pState->readKey = false;
		SetWindowTextA(pState->hKey, ("Key: " + std::to_string(pState->keyCode)).c_str());
		break;
	case WM_CREATE: {
		CREATESTRUCTW *pCraete = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<QueueActionState*>(pCraete->lpCreateParams);
		pState->keyCode = 0;
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);
		addUI(hWnd, pState);
	}	break;
	case WM_HOTKEY:
			PostMessage(GetParent(hWnd), WM_COMMAND, COM_EXECUTE_EVENT, (LPARAM)hWnd);
		break;
	case WM_KEYDOWN:
		if (pState->readKey) {
			if (pState->keyCode != wParam) {
				if (pState->keyCode) { if (!UnregisterHotKey(hWnd, 1)) showError("UnRegsiterHotKey", GetLastError()); }
				pState->keyCode = wParam;
				if (pState->keyCode) { if (!RegisterHotKey(hWnd, 1, 0, wParam)) showError("RegisterHotKey" ,GetLastError()); }
				SetWindowTextA(pState->hKey, ("Key: " + std::to_string(pState->keyCode)).c_str());
			}
			pState->readKey = false;
		}
		break;
	case WM_DESTROY:
		if (pState) delete pState;
		break;
	case WM_COMMAND:
		com = wParam;
		break;
	default: return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	switch (com) {
	case COM_CLOSE: PostMessageW(GetParent(hWnd), WM_DESTROYCHILD, NULL, (LPARAM)hWnd); DestroyWindow(hWnd); break;
	case COM_ADDEVENT: PostMessageW(GetParent(hWnd), msg, wParam, (LPARAM)hWnd); break;
	case COM_READKEY: pState->readKey = true; SetWindowTextA(pState->hKey, "---"); SetFocus(hWnd); break;
	}
	return 0;
}

LRESULT CALLBACK AddressBookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	AddressBookState* pState = reinterpret_cast<AddressBookState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_CREATE: {
		CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<AddressBookState*>(pCreate->lpCreateParams);
		pState->scroll = 0;
		pState->parent = pCreate->hwndParent;
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);
		addUI(hWnd, pState);
	} break;
	case WM_CLOSE:
		PostMessageW(pState->parent, WM_DESTROYCHILD, NULL, (LPARAM)hWnd);
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY: {
		if (pState) delete pState;
	} break;
	case WM_COMMAND: {
		if (!PostMessageW(pState->parent, WM_COMMAND, COM_SELECTITEM, wParam - 1))
			showError("PostMes", GetLastError());
		DestroyWindow(hWnd);
	} break;
	case WM_VSCROLL: {
		WORD low = LOWORD(wParam), high = HIWORD(wParam);
		int diff = 0;
		bool redraw = true;
		switch (low)
		{
		case SB_THUMBTRACK: diff = high - pState->scroll; redraw = false; break;
		case SB_THUMBPOSITION: diff = high - pState->scroll; break;
		case SB_LEFT: diff = -pState->scroll; break;
		case SB_RIGHT: diff = pState->addresses.size() * DIM_ADDRESSLINE.boundH; break;
		case SB_LINELEFT: diff = -DIM_ADDRESSLINE.boundH; break;
		case SB_LINERIGHT: diff = DIM_ADDRESSLINE.boundH; break;
		case SB_PAGELEFT: diff = -pState->linePerBook * DIM_ADDRESSLINE.boundH; break;
		case SB_PAGERIGHT: diff = pState->linePerBook * DIM_ADDRESSLINE.boundH; break;
		}
		if (diff != 0) {
			int maxD = (pState->addresses.size() - pState->linePerBook) * DIM_ADDRESSLINE.boundH - pState->scroll,
				minD = -pState->scroll;
			if (diff > maxD) diff = maxD;
			if (diff < minD) diff = minD;
			if (diff != 0) {
				pState->scroll += diff;
				SetScrollPos(hWnd, SB_VERT, pState->scroll, redraw);
				ScrollWindow(hWnd, 0, -diff, NULL, NULL);
			}
		}
	} break;
	default: return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	return 0;
}