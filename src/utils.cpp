#include "utils.hpp"

namespace Utils {

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