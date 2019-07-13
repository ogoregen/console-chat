#define LIBRG_IMPLEMENTATION

#include "librg.h"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>

librg_ctx ctx = { 0 };
std::vector<std::string> peerNames;
u32 id = 0;

void onRequest(librg_event* event){

	u32 pass = librg_data_ru32(event->data);
	if(pass != 871797) librg_event_reject(event);
}

void onAcception(librg_event* event){

	std::cout << (unsigned int)event->peer << " connected. ";
	librg_message_send_to(&ctx, LIBRG_EVENT_LAST + 2, event->peer, &id, sizeof(id)); //send to the client the ID assigned to them
	id++;

	std::ifstream file;
	file.open("history.txt");
	std::string history, line;
	while(std::getline(file, line)) history += line;
	file.close();

	librg_data* data = librg_data_init_new();
	u32 length = history.length() + 1;
	librg_data_wu32(data, length);
	librg_data_wptr(data, &history[0], sizeof(char) * length);
	librg_message_send_to(&ctx, LIBRG_EVENT_LAST + 3, event->peer, data->rawptr, librg_data_get_wpos(data)); //send the message history to the client
	librg_data_free(data);
}

void onRefusal(librg_event* event){

	std::cout << "Connection refused" << std::endl;
}

void onMessage2(librg_message* message){ 

	u32 id = librg_data_ru32(message->data);
	u32 length = librg_data_ru32(message->data);
	char* incoming = new char[length];
	librg_data_rptr(message->data, incoming, sizeof(char) * length);

	std::cout << peerNames[id] << ": " << incoming << std::endl;

	librg_data* data = librg_data_init_new();
	librg_data_wu32(data, peerNames[id].length() + 1); //length of sender name
	librg_data_wptr(data, &peerNames[id][0], sizeof(char) * (peerNames[id].length() + 1)); //sender name
	librg_data_wu32(data, length); //lendth of message
	librg_data_wptr(data, incoming, sizeof(char) * length); //message
	librg_message_send_except(&ctx, LIBRG_EVENT_LAST + 1, message->peer, data->rawptr, librg_data_get_wpos(data)); //redirect the message to every peer except the sender
	librg_data_free(data);

	std::fstream file;
	file.open("history.txt", std::ios_base::app);
	file << peerNames[id] + ": " + incoming + 'Ð'; //add the message to the history
	file.close();
}

void onMessage1(librg_message* message){ //receive name

	u32 length = librg_data_ru32(message->data);
	char* incoming = new char[length];
	librg_data_rptr(message->data, incoming, sizeof(char) * length);
	
	peerNames.push_back(incoming);
	std::cout << (unsigned int)message->peer << "'s name is " << incoming << std::endl;

	std::string m = "";
	m += incoming;
	m += " connected. ";
	m += '\n';
	librg_data* data = librg_data_init_new();
	u32 length2 = m.length() + 1;
	librg_data_wu32(data, length2);
	librg_data_wptr(data, &m[0], sizeof(char) * length2);
	librg_message_send_except(&ctx, LIBRG_EVENT_LAST + 3, message->peer, data->rawptr, librg_data_get_wpos(data));
	librg_data_free(data);
}

int main(){

	ctx.mode = LIBRG_MODE_SERVER;
	librg_init(&ctx);

	librg_event_add(&ctx, LIBRG_CONNECTION_REQUEST, onRequest);
	librg_event_add(&ctx, LIBRG_CONNECTION_ACCEPT, onAcception);
	librg_event_add(&ctx, LIBRG_CONNECTION_REFUSE, onRefusal);
	librg_network_add(&ctx, LIBRG_EVENT_LAST + 1, onMessage1); //register names
	librg_network_add(&ctx, LIBRG_EVENT_LAST + 2, onMessage2); //receive messages

	librg_address address = { 7779 };
	librg_network_start(&ctx, (librg_address)address);

	std::cout << "Server started." << std::endl;
	while(1) librg_tick(&ctx);
	librg_network_stop(&ctx);
	librg_free(&ctx);
	return 0;
}
