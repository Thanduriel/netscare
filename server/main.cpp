#include "Network.hpp"
#include <iostream>


// http resquets GET -> Download side from netscare
// http request POST t=login&name=str d=pKey-> t=login&failed=0|1 d=null|sKey
// http request POST t=loadPic&userId&num d=secrat,picId,picdata -> t=recived&t&num&size=int
// http request POST t=update&userId&num d=secret,data -> t=success&t&num&size=int d=secret,data

// data cli -> ser array of events with {target, picId, state one Nothing, SetUp, trigger, Cancle}
// data ser -> cli array of events with {target, picId, staet one of ready, NoPic, PicLoadingTarget, Canceld, executed} array of address {id, online/oofline, tickets}


int __cdecl wmain(int argc, wchar_t **argv) {
	if (argc < 2) {
		wprintf(L"%s: <Url1> [Url2] ... \n", argv[0]);
		return -1;
	}
	NetScareServer server(argc, argv);
	do {
		Sleep(100);
		server.updateServer();
	} while (true);
}