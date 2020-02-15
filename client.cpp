
#define LIBRG_IMPLEMENTATION

#include "librg.h"
#include <string>
#include <iostream>
#include <fstream>
#include <thread>

bool connected = 0;
librg_ctx ctx = { 0 };
std::string message, name;
u32 id;

void onRequest(librg_event* event){

  librg_data_wu32(event->data, 871797);
  std::cout << "Requesting connection" << std::endl;
}

void onAcception(librg_event* event){

  std::cout << "Connection accepted" << std::endl << std::endl;
  connected = 1;
}

void onRefusal(librg_event* event){

  std::cout << "Connection refused" << std::endl;
}

void onMessage(librg_message* message){ //receive messages

  u32 nameLength = librg_data_ru32(message->data);
  char* incomingName = new char[nameLength];
  librg_data_rptr(message->data, incomingName, sizeof(char) * nameLength);

  u32 length = librg_data_ru32(message->data);
  char* incoming = new char[length];
  librg_data_rptr(message->data, incoming, sizeof(char) * length);

  std::cout << incomingName << ": " << incoming << std::endl;
}

void onMessage2(librg_message* message){ //receive client ID

  id = librg_data_ru32(message->data);
}

void onMessage3(librg_message* message){ //receive messages to print directly

  u32 length = librg_data_ru32(message->data);
  char* incoming = new char[length];
  librg_data_rptr(message->data, incoming, sizeof(char) * length);

  std::string h(incoming);

  for(char c: h){

    if(c == 'Ð') std::cout << std::endl; //not the ideal way, but it works
    else std::cout << c;
  }
}

void query(){

  while(1){

    std::string input;
    std::getline(std::cin, input);

    if(input != ""){

      librg_data* data = librg_data_init_new();
      librg_data_wu32(data, id); //sender ID
      librg_data_wu32(data, input.length() + 1); //message length
      librg_data_wptr(data, &input[0], sizeof(char) * (input.length() + 1)); //message
      librg_message_send_all(&ctx, LIBRG_EVENT_LAST + 2, data->rawptr, librg_data_get_wpos(data));
      librg_data_free(data);
    }
  }
}

int main(){

  std::ifstream file;
  file.open("data.txt");
  file >> name;
  file.close();

  if(name == ""){

    std::cout << "Who is calling? ";
    std::getline(std::cin, name);
    std::cout << std::endl;
    std::ofstream file;
    file.open("data.txt");
    file << name;
    file.close();
  }
  else std::cout << "Welcome back, " << name << "!" << std::endl << std::endl;

  ctx.mode = LIBRG_MODE_CLIENT;
  librg_init(&ctx);

  librg_event_add(&ctx, LIBRG_CONNECTION_REQUEST, onRequest);
  librg_event_add(&ctx, LIBRG_CONNECTION_ACCEPT, onAcception);
  librg_event_add(&ctx, LIBRG_CONNECTION_REFUSE, onRefusal);
  librg_network_add(&ctx, LIBRG_EVENT_LAST + 1, onMessage);
  librg_network_add(&ctx, LIBRG_EVENT_LAST + 2, onMessage2);
  librg_network_add(&ctx, LIBRG_EVENT_LAST + 3, onMessage3);

  const char* ip = "127.0.0.1";
  librg_address address;
  address.host = (char*)ip;
  address.port = 7779;
  librg_network_start(&ctx, (librg_address)address);

  while(!connected) librg_tick(&ctx); //wait for the connection

  librg_data* data = librg_data_init_new(); 
  librg_data_wu32(data, name.length() + 1);
  librg_data_wptr(data, &name[0], sizeof(char) * (name.length() + 1));
  librg_message_send_all(&ctx, LIBRG_EVENT_LAST + 1, data->rawptr, librg_data_get_wpos(data)); //send user name to the server
  librg_data_free(data);

  std::thread first(query);   
  while(1) librg_tick(&ctx);
  first.join();
  librg_network_stop(&ctx);
  librg_free(&ctx);
  return 0;
}

