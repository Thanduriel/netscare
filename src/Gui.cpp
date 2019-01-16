#include "Gui.hpp"

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

Gui::Gui(HINSTANCE hInst) {
	WNDCLASSW winC = { 0 };
	winC.hCursor = LoadCursor(NULL, IDC_ARROW);
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"MainWindow";
	winC.lpfnWndProc = MainWinProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	winC = {0};
	winC.hCursor = LoadCursor(NULL, IDC_ARROW);
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"QueueBoxWin";
	winC.lpfnWndProc = QueueBoxProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	winC = { 0 };
	winC.hCursor = LoadCursor(NULL, IDC_ARROW);
	winC.hbrBackground = (HBRUSH)COLOR_WINDOW;
	winC.hInstance = hInst;
	winC.lpszClassName = L"QueueWinWin";
	winC.lpfnWndProc = QueueWinProc;
	if (!RegisterClassW(&winC)) {
		UnmapDll();
		return;
	}

	{
		MainWinState *pState = new (std::nothrow) MainWinState{};
		_hInput = CreateWindowW(L"MainWindow", L"NetScare", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 500, NULL, NULL, NULL, pState);
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

void addUI(HWND hWnd, QueueWinState* pState) {
	CreateWindowW(L"button", L"X", WS_VISIBLE | WS_CHILD, 1, 1, 10, 10, hWnd, (HMENU)COM_CLOSE, NULL, NULL);
	wchar_t key[10];
	swprintf_s(key, L"%d", pState->keyCode);
	CreateWindowW(L"static", key, WS_VISIBLE | WS_CHILD, 1, 20, 30, 20, hWnd, NULL, NULL, NULL);
}

LRESULT CALLBACK MainWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	MainWinState *pState = reinterpret_cast<MainWinState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_CREATE: {
		CREATESTRUCTW *pCreat = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<MainWinState*>(pCreat->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pState);
		addUI(hWnd, pState);
	}
					break;
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
	case WM_DESTROY:
		delete pState;
		PostQuitMessage(0);
		break;
		return 0;
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
		PostMessageW(pState->hQueueBox, WM_COMMAND, COM_ADDQUEUE, NULL);
		break;
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
			if (pState->hQueues[i] == delet) continue;
			if (num < i) {
				RECT rect;
				GetWindowRect(pState->hQueues[i], &rect);
				int w = rect.right - rect.left,
					h = rect.bottom - rect.top;
				rect.top -= self.top + 1;
				rect.left -= self.left + (i - num) * DIM_QUEUEWIN.boundW + 1;
				MoveWindow(pState->hQueues[i], rect.left, rect.top, w, h, TRUE);
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
	case WM_COMMAND:
		com = wParam;
		break;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	switch (com) {
	case COM_ADDQUEUE: {
		static int num = 0;
		QueueWinState * state = new (std::nothrow) QueueWinState;
		state->keyCode = num;
		++num;
		pState->hQueues.push_back(CreateWindowW(L"QueueWinWin", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
			DIM_QUEUEWIN.x + DIM_QUEUEWIN.boundW * pState->hQueues.size(), DIM_QUEUEWIN.y, DIM_QUEUEWIN.width, DIM_QUEUEWIN.height,
			hWnd, NULL, NULL, state));
		if (!pState->hQueues.back())
			showError("PQueueError", GetLastError());
		if (pState->hQueues.size() > QUEUE_WIN_PRO_BOX)
			SetScrollRange(hWnd, SB_HORZ, 0, (pState->hQueues.size() - QUEUE_WIN_PRO_BOX) * DIM_QUEUEWIN.boundW, true);
	}	break;
	}

	return 0;
}

LRESULT CALLBACK QueueWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	int com = 0;
	QueueWinState* pState = reinterpret_cast<QueueWinState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	switch (msg) {
	case WM_DESTROY: PostMessageW(GetParent(hWnd), WM_DESTROYCHILD, NULL, (LPARAM)hWnd); break;
	case WM_CREATE: {
		CREATESTRUCTW *pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		pState = reinterpret_cast<QueueWinState*>(pCreate->lpCreateParams);
		addUI(hWnd, pState);
	} break;
	case WM_COMMAND: com = wParam;
		break;
	default: return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	switch (com) {
	case COM_CLOSE:
		DestroyWindow(hWnd);
		break;
	}
	return 0;
}