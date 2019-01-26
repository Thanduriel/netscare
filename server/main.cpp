#include "Network.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <io.h>


// http resquets GET -> Download side from netscare
// http POST string:login,OctasString:Name,STRUCT:publicKey -> OctaString:msg ~> string:denied|access,Struct:symKey,Struct:macKey,Integer:uid
// http POST Integer:uid,STRUCT:macTag,OctaString:msg~>Integer:hash,struct:{string:pic, Intgeer:id, OcatString:ddsfile}|{string:update, struct{string:prep, I:target, I:picId, I:eventId}|struct{string:fire, I:target, I:eventId} }
// -> Strcut:mactag,OcatString:msg~>Integer:hash,struct{OcatSTring:tickets, BisString:eventPrep(y/n)|eventExecuted(y/n)}
// http POST Integer:uuid,Struct:maxTag, OCatSTring:msg~>Integer:hash,strcut:{string:names, OctaSTring:ArrayOfNames in wchar o terminiert}

constexpr int MAX_PICTURES_PER_USER = 20;


int __cdecl wmain(int argc, wchar_t **argv) {
	if (argc < 2) {
		wprintf(L"%s: <Url1> [Url2] ... \n", argv[0]);
		return -1;
	}

	Tickets tickets(20);
	std::vector<User> users{};

	NetScareServer server(argc, argv, users, tickets);
	NetScareServer::Action action;
	do {
		Sleep(100);
		server.UpdateServer(action);
		switch (action.type) {
		case NetScareServer::Action::USER_NEW:
			/* users.push_back(User(reinterpret_cast<wchar_t*>(action.data)));
			tickets.AddUser();
			delete[] action.data; */
			break;
		case NetScareServer::Action::PIC_LOAD: {
			/* std::uint8_t user = action.data[0], pId = action.data[1];
			std::size_t len = *reinterpret_cast<std::size_t*>(action.data + 2);
			char fName[32];
			sprintf_s(fName, "%d_%d.dds", user, pId);
			bool picUpdate = _access(fName, 0) == 0;
			std::ofstream file(fName);
			file.write(action.data + 2 + sizeof(std::size_t), len);
			file.close();
			if (!picUpdate) {
				users[user].LoadPic(pId);
			} else {
				users[user].UpdatePic(pId);
			}
			delete[] action.data; */
		}	break;
		case NetScareServer::Action::EVENT_ADD: {
			/* char name[25];
			sprintf_s(name, "%d_%d.dds", action.data[0], action.data[1]);
			users[action.data[0]].AddEvent(action.data[0], action.data[1], *reinterpret_cast<std::uint8_t*>(action.data + 2), _access(name, 0));
			delete[] action.data; */
		}	break;
		}
	} while (true);
}