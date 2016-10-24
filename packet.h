#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdlib.h>

#include <netinet/in.h> //sockaddr_in struct

#include "list.h"

#define PACKET_HEADER_SIZE 4
#define PACKET_SIZE (PACKET_HEADER_SIZE + sizeof(void*))

#define MAJOR_VER 0
#define MINOR_VER 1

//Package Types
enum PacketType {
	PKT_PING = 0,
	PKT_HELLO,
	PKT_HELLO_ERR,
	PKT_MSG,
	PKT_NICKCHANGE,
	PKT_GOODBYE
};

//Generic Packet Structure
struct PacketHeader {
  uint8_t type;
  uint8_t msg_size;
  uint16_t p_id;
};

struct Packet {
	struct PacketHeader header;
	void* payload;
};

void PKT_Free(struct Packet *p);
struct Packet *PKT_Copy(struct Packet *p);

struct Packet *recvPacket(int sock, struct sockaddr_in *r_addr, unsigned short block);
int sendPacket(int sock, struct Packet *p, struct sockaddr_in *r_addr); //0 - Ok <0 - Error >0 - Errno is set



//PACKET PAYLOADS
struct PktHello {
  uint8_t major;
  uint8_t minor;
};

enum PacketHelloError {
	HERR_VERMISMATCH=0,
	HERR_CONNECTED
};

struct PktHelloErr {
  uint8_t err_c;
};

struct PktMsg {
	char *msg;
};

struct PktAuthChallenge {
	char rand_bytes[12];
	
};

struct PktAuthResponse {
	char resp_bytes[12];
};

struct PktNickChange {
	char *name;	
};


#endif
