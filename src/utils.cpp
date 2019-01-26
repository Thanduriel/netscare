#include "utils.hpp"
#include "spdlog/sinks/basic_file_sink.h"

namespace Utils {

	std::unique_ptr<spdlog::logger> logger;

	spdlog::logger& Log()
	{
		// msvc stl should have debug checks when uninitialized
		return *logger;
	}

	void InitLogger(const std::filesystem::path& _file)
	{
		logger = std::make_unique<spdlog::logger>("default", 
			std::make_shared<spdlog::sinks::basic_file_sink_st>(_file.string()));
	}

	struct WindowData {
		unsigned long pid;
		HWND hWnd;
	};

	BOOL IsMainHandle(HWND handle) 
	{
		return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
	}

	BOOL CALLBACK find_window_callback(HWND handle, LPARAM lParam) 
	{
		WindowData& wnd = *reinterpret_cast<WindowData*>(lParam);
		unsigned long pid;
		GetWindowThreadProcessId(handle, &pid);
		if (wnd.pid != pid || !IsMainHandle(handle))
			return TRUE;

		wnd.hWnd = handle;
		return FALSE;
	}

	HWND GetProcessWindow(DWORD _processID)
	{
		WindowData myWnd;
		myWnd.pid = _processID;
		EnumWindows(find_window_callback, (LPARAM)&myWnd);

		return myWnd.hWnd;
	}


	void ShowError(const char* caption, DWORD error)
	{
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

}