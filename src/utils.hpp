#include <Windows.h>
#include <filesystem>
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include "spdlog/spdlog.h"

namespace Utils {

	// InitLogger() needs to be called once before this can be used!
	spdlog::logger& Log();
	void InitLogger(const std::filesystem::path& _file);

	HWND GetProcessWindow(DWORD _processID);

	void ShowError(const char* caption, DWORD error);

	template<typename T, typename... Args>
	void ResetInplace(T& _obj, Args&&... _args)
	{
		_obj.~T();
		new (&_obj) T(std::forward<Args>(_args)...);
	}
}