#include "Network.hpp"

int __cdecl wmain(int argc, wchar_t **argv) {
	if (argc < 2) {
		wprintf(L"%s: <Url1> [Url2] ... \n", argv[0]);
		return -1;
	}
	runServer(argc, argv);
}