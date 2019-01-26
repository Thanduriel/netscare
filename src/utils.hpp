#include <Windows.h>

namespace Utils {

	void ShowError(const char* caption, DWORD error);

	template<typename T, typename... Args>
	void ResetInplace(T& _obj, Args&&... _args)
	{
		_obj.~T();
		new (&_obj) T(std::forward<Args>(_args)...);
	}
}