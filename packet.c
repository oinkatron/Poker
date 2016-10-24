#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "packet.h"
#include "util.h"


#define SERIALIZE_PACKET_TYPE(p,d,r) (d) = (char*)malloc(PACKET_HEADER_SIZE + sizeof(r));\
				  (d)[0] = (char)(p)->header.type;\
				  (d)[1] = (char)(p)->header.msg_size;\
				  *(uint16_t*)((d)+2) = (p)->header.p_id;\
				  memcpy((d)+PACKET_HEADER_SIZE, (p)->payload, sizeof(r))

#define UNSERIALIZE_PACKET_TYPE(p,d,r)  (p) = (struct Packet*)malloc(sizeof(struct Packet));\
					(p)->header.type = (d)[0];\
					(p)->header.msg_size = (d)[1];\
					(p)->header.p_id = *(uint16_t*)((d)+2);\
					(p)->payload = (r*)malloc(sizeof(r));\
					memcpy((p)->payload, (d)+4, sizeof(r))

char* serializePacket(struct Packet *p) {
	char *data = NULL;
	switch(p->header.type) {
		case PKT_PING:
			data = (char*)malloc(PACKET_HEADER_SIZE);
			
			data[0] = (char)p->header.type;
			data[1] = 0;
			*((uint16_t*)(data+2)) = p->header.p_id;
			
			return data;
		case PKT_HELLO:
			SERIALIZE_PACKET_TYPE(p,data,struct PktHello);
			/*data = (char*)malloc(PACKET_HEADER_SIZE+sizeof(struct PktHello));
			data[0] = (char)p->header.type;
			data[1] = (char)p->header.msg_size;
			*(uint16_t*)(data+2) = p->header.p_id;

			memcpy((data+PACKET_HEADER_SIZE), p->payload, sizeof(struct PktHello));*/
	
			return data;
		case PKT_HELLO_ERR:
			/*data = (char*)malloc(PACKET_HEADER_SIZE+sizeof(struct PktHelloErr));
			//serialize header
			data[0] = (char)p->header.type;
			data[1] = (char)p->header.msg_size;
			*((uint16_t*)(data+2)) = p->header.p_id;

			memcpy((data+PACKET_HEADER_SIZE), p->payload, sizeof(struct PktHelloErr));
			*/
			SERIALIZE_PACKET_TYPE(p,data,struct PktHelloErr);
			return data;
		default:
			printf("Unknown packet type!\n");
			return NULL;
	}
}


struct Packet *unserializePacket(char *buff) {
	struct Packet *new_p = NULL;	
	enum PacketType p_type = buff[0];
	uint8_t msg_l = buff[1];
	uint16_t msg_id = *(uint16_t*)(buff+2);

	switch (p_type) {
		case PKT_PING:
			if (msg_l != 0) {
				printf("Pings have no payload!\n");
				break;
			}
			
			new_p = (struct Packet*)malloc(sizeof(struct Packet));
			
			new_p->header.type = p_type;
			new_p->header.msg_size = 0;
			new_p->header.p_id = msg_id;

			return new_p;
		
		case PKT_HELLO:
			if ((uint8_t)sizeof(struct PktHello) != msg_l) {
				printf("Error wrong size: %d instead of %d\n", msg_l, (int)sizeof(struct PktHello));
				break;
			}
			
			UNSERIALIZE_PACKET_TYPE(new_p, buff, struct PktHello);

			/*new_p = (struct Packet*)malloc(sizeof(struct Packet));

			new_p->header.type = p_type;
			new_p->header.msg_size = msg_l;
			new_p->header.p_id = msg_id;
			
			new_p->payload = malloc(sizeof(struct PktHello));
			memcpy(new_p->payload, buff+4, sizeof(struct PktHello));
			*/
			return new_p;
		case PKT_HELLO_ERR:
			if ((uint8_t)sizeof(struct PktHelloErr) != msg_l) {
				printf("Error wrong size: %d instead of %d\n", msg_l, (int)sizeof(struct PktHello));
				break;
			}
			
			UNSERIALIZE_PACKET_TYPE(new_p, buff, struct PktHelloErr);

			return new_p;
		default: 
			printf("%s\n", "Unknown Packet type");
			break;
	}

	return NULL;

} 

struct Packet *recvPacket(int sock, struct sockaddr_in *r_addr, unsigned short block) {
	char buff[256];
	memset(buff, 0, 256);

	int n =0;
	socklen_t r_len = sizeof(struct sockaddr_in);

	struct Packet *new_p = NULL;
	if (block)
		n = recvfrom(sock, buff, 255, 0, (struct sockaddr*)r_addr, &r_len);
	else 
		n = recvfrom(sock, buff, 255, MSG_DONTWAIT, (struct sockaddr*)r_addr, &r_len);
	
	if (n < 0) {
		if (errno == EAGAIN) {
			return NULL;
		} else {
			perror("UDP Recv Error");
			return NULL;
		}
	} else if (n == 256) {
		printf("Message truncated!");
		return NULL;
	} else {
		return unserializePacket(buff);
	}	
}

int sendPacket(int sock, struct Packet *p, struct sockaddr_in *r_addr) {
	char *pkt = serializePacket(p);
	socklen_t r_len = sizeof(struct sockaddr_in);

	if (pkt) {
		int n = sendto(sock, pkt, PACKET_HEADER_SIZE + pkt[1], 0, (struct sockaddr*)r_addr, r_len);
		if (n < 0) {
			int err = errno;
			perror("Error sending data");
			return errno;
		}

		return 0;
	}

	return -1;
}

struct Packet *PKT_Copy(struct Packet *p) {
	if (p->header.msg_size > 255) {
		printf("Packet size too large!\n");
		return NULL;
	}
	struct Packet *n = (struct Packet*)malloc(sizeof(struct Packet));

	n->header = p->header;	
	n->payload = (void*)malloc(p->header.msg_size);
	memcpy(n->payload, p->payload, p->header.msg_size);

	return n;

}

void PKT_Free(struct Packet *p) {
	if (p) {
		if (p->payload) {
			free(p->payload);
		}
	}

	free(p);
	return;
}

