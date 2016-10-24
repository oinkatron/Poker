#include "udp.h"

void PktFree(struct RecvPckt *p) {
		if (p) {
			free(p->payload);
			free(p);
		}
}

struct Server *ServCreate(char *addr, short portno, short max_c, unsigned short buff_size) {
	int _sock=0;
	struct sockaddr_in _addr;

	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(portno);
	if (inet_aton(addr, (struct in_addr*)&_addr.sin_addr.s_addr) == 0) {
		printf("Error resolving dotted address: %s\n", addr);
		return NULL;
	}
   
	_sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (_sock <= 0) {
		perror("Socket system call failed");
		return NULL;
	}

	if (bind(_sock, (struct sockaddr*)&_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("Failed to bind socket");
		close(_sock);
		return NULL;
	}

	struct Server *new_c = malloc(sizeof(struct Server));
	new_c->sock = _sock;
	new_c->b_addr = _addr;

	memset(new_c->name, 0, 32);
	memset(new_c->password_str, 0, 64);

	new_c->start_time = millitime();
	new_c->cur_time = new_c->start_time;

	new_c->max_cli = max_c;
	new_c->clients = malloc(sizeof(struct CliData)*max_c);

	new_c->recv_buff = malloc(buff_size);
	memset(new_c->recv_buff, 0, buff_size);
	
	new_c->max_len = buff_size;
	new_c->c_timeout = 1000;

	return new_c;
}

void ServFree(struct Server *s) {
	if (s) {
		close(s->sock);
		free(s->recv_buff);
		free(s->clients);
		free(s);
	}
}

int ServPcktSend(struct Server *s, struct CliData *c, struct Pckt *p) {
	if (!s || !c || !p) {
		return -1;
	}

	if (p->size + PACKET_HEAD_SIZE > s->max_len) {
		printf("Payload to large for connection: %d of %d bytes\n", p->size+PACKET_HEAD_SIZE, s->max_len);
		return -1;
	}

	char *msg_buff = malloc(PACKET_HEAD_SIZE+p->size);

	//setup header
	*(short*)msg_buff = 0xFEFF;
	*(char*)(msg_buff+2) = p->flags; //flags
	*(long*)(msg_buff+3) = c->id;
	*(int*)(msg_buff+11) = c->l_seq++;
	*(int*)(msg_buff+15) = c->ack_seq;
	*(int*)(msg_buff+19) = c->ack_bits;
	*(short*)(msg_buff+23) = p->size;
		
	//copy message if needed
	if (p->payload && p->size > 0) {  
		memcpy(msg_buff+PACKET_HEAD_SIZE, p->payload, p->size);
	}

	int n = sendto(s->sock, msg_buff, p->size+PACKET_HEAD_SIZE, 0, (struct sockaddr*)&c->addr, sizeof(c->addr));
	if (n == p->size+PACKET_HEAD_SIZE) {
		//sent successfully
		c->n_sent++;
		p->sent_t = s->cur_time;

		if (c->ack_cnt < ACK_BUFF_LEN) {
			memcpy(&c->ack_buff[c->ack_cnt++], p, sizeof (struct RecvPckt)); //push another pckt onto ack buffer
		} else {
			printf("Packet [%d] dropped!\n", c->ack_buff[0].seq_id);
			//CALLBACK HERE
			memmove(c->ack_buff, &c->ack_buff[1], sizeof(struct Pckt) * (ACK_BUFF_LEN-1));
			memcpy(&c->ack_buff[ACK_BUFF_LEN-1], p, sizeof(struct Pckt));
		}

		free(msg_buff);
		return 0;
	}


	free(msg_buff);
	return errno;
}


int ServSend(struct Server *s, struct CliData *c, char f, char *buff, unsigned short buff_len) {
	if (!s || !c) {
		return -1;
	}

	if (buff_len+PACKET_HEAD_SIZE > s->max_len) {
		printf("Payload to large for connection: %d of %d bytes\n", buff_len+PACKET_HEAD_SIZE, s->max_len);
		return -1;
	}


	int n=0;
	socklen_t a_len = sizeof(c->addr);

	//Create the sending buffer
	char *msg_buff = (char*)malloc(PACKET_HEAD_SIZE + buff_len);

	//Prepare the ConnHeader portioni
	*(short*)msg_buff = 0xFEFF;
	*(char*)(msg_buff+2) = f; //flags
	*(long*)(msg_buff+3) = c->id;
	*(int*)(msg_buff+11) = ++c->l_seq;
	*(int*)(msg_buff+15) = c->ack_seq;
	*(int*)(msg_buff+19) = c->ack_bits;
	*(short*)(msg_buff+23) = buff_len;
		
	//copy message
	if (buff && buff_len > 0) {  
		memcpy(msg_buff+PACKET_HEAD_SIZE, buff, buff_len);
	} else {
		*(short*)(msg_buff+23) = 0;
		buff_len = 0;
	}


	n = sendto(s->sock, msg_buff, (PACKET_HEAD_SIZE+buff_len), 0, (struct sockaddr*)&c->addr, a_len);
	if (buff_len+PACKET_HEAD_SIZE == n) { 
		//successfully sent
		c->n_sent++;
		
		//copy payload and flags to pckt struct
		if (c->ack_cnt < ACK_BUFF_LEN) {
			c->ack_buff[c->ack_cnt].flags = f;
			c->ack_buff[c->ack_cnt].sent_t = s->cur_time;
			c->ack_buff[c->ack_cnt].seq_id = c->l_seq;
			c->ack_buff[c->ack_cnt].size = buff_len;

			if (buff_len > 0  && buff) {
				c->ack_buff[c->ack_cnt].payload = malloc(buff_len);
				memcpy(c->ack_buff[c->ack_cnt].payload, buff, buff_len);
			} else {
				c->ack_buff[c->ack_cnt].payload = NULL;
			}

			++c->ack_cnt;
		} else {
			//drop this pckt
			printf("Packet [%d] dropped!\n", c->ack_buff[0].seq_id);
			//CALLBACK HERE
			memmove(c->ack_buff, &c->ack_buff[1], sizeof(struct Pckt) * (ACK_BUFF_LEN-1));
			
			c->ack_buff[c->ack_cnt-1].flags = f;
			c->ack_buff[c->ack_cnt-1].sent_t = s->cur_time;
			c->ack_buff[c->ack_cnt-1].seq_id = c->l_seq;
			c->ack_buff[c->ack_cnt-1].size = buff_len;
			
			if (buff_len > 0 && buff) {
				c->ack_buff[c->ack_cnt-1].payload = malloc(buff_len);
				memcpy(c->ack_buff[c->ack_cnt-1].payload, buff, buff_len);
			} else {
				c->ack_buff[c->ack_cnt-1].payload = NULL;
			}

		}
		
		free(msg_buff);	
		return n;

	} else {
		free(msg_buff);
		return 0;
	}

	return 0;
}

struct RecvPckt* ServRecv(struct Server *s, int timeout) {
	if(!s) {
		return NULL;
	}
	
	int opts = 0;
	int n = 0;
	
	struct RecvPckt *new_p = malloc(sizeof(struct RecvPckt));
	memset(new_p, 0, sizeof(struct RecvPckt));

	socklen_t r_len = sizeof(new_p->origin);	

	if (timeout <= 0) { 
		opts |= MSG_DONTWAIT;
	} else {
		setGlobalTimeout(s->sock, timeout);
	}
	
	//flush recieving buffer
	memset(s->recv_buff, 0, s->max_len);

	//recieve raw packet
	n = recvfrom(s->sock, s->recv_buff, s->max_len, opts,
						 (struct sockaddr*)&new_p->origin, &r_len);
	if (n > 0 && n < s->max_len) {
		//if successful extract data
		printf("Recieved Packet: %d bytes\n", n);

		char *msg_buff = s->recv_buff;
		
		if (msg_buff[0] == -2 && msg_buff[1] == -1) { //proper order
			printf("Reversed byte order TODO (BIG endian)\n");
		} else if ((int)msg_buff[0] == -1 && (int)msg_buff[1] == -2) { //convert from stupid byte order
			new_p->flags = msg_buff[2];
			new_p->cli_id = *(long*)(msg_buff+3);
			new_p->seq_id = *(int*)(msg_buff+11);
			new_p->ack_seq = *(int*)(msg_buff+15);
			new_p->ack_bits = *(int*)(msg_buff+19);
			new_p->size = *(short*)(msg_buff+23);


			printf("Recieved ack bits: %d\n", new_p->ack_bits);

			//extract packet
			new_p->payload = malloc(new_p->size);
			memcpy(new_p->payload, msg_buff+PACKET_HEAD_SIZE, new_p->size);
		}
	   
		return new_p;

	} else if (errno != EAGAIN) {
			return NULL; //its ok timedout
	} else {
			return NULL;
	}

	return NULL;

}

void ServSendInfo(struct Server *s, struct sockaddr_in *r) {
	if (!s || !r) {
		return;
	}

	int data_len = 6 + strlen(s->name);
	char data[data_len];

	//PASS_PROTECT + VER MIN + VER MAX + MIN
	if (strlen(s->password_str) > 0) {
		data[0] = 1;
	} else {
		data[0] = 0;
	}
	
	data[1] = VERSION_MINOR;
	data[2] = VERSION_MAJOR;

	data[3] = s->num_cli;
	data[4] = s->max_cli;
	data[5] = data_len-6;
	if (data[5] > 0) {
		strcpy(&data[6], s->name);
	}

	//send the data to the client
	int n = sendto(s->sock, data, data_len, 0, (struct sockaddr*)r, sizeof(struct sockaddr_in));
	if (n != data_len) {
		//Print an error
	}
}

void ServProcessPckt(struct Server *s, struct CliData *c, struct RecvPckt *p) {
	if (!c || !p) {
		return;
	}

	c->n_recvd++;

	//push packets id onto ack buffer to send next time
	int res = p->seq_id - c->ack_seq;
	if (res >= 1 && res <= 32) { //newer sequenced packet
		c->ack_bits = (c->ack_bits << 1) | 1;
		c->ack_bits = (c->ack_bits << (res-1));
		c->ack_seq = p->seq_id;
	} else if (res < 0 && res >= -32) { //packet is older
		printf("Older packet recieved!");
		c->ack_bits |= 1 << (c->ack_seq - p->seq_id - 1);
	} else if (res > 32) {
		c->ack_bits = 0;
		c->ack_seq = p->seq_id;
	}

	//check recieved ack against unacked packets
	if (c->ack_cnt > 0) {
		for (int i=c->ack_cnt-1; i >= 0; --i) {
			if (c->ack_buff[i].seq_id == p->ack_seq || (p->ack_seq - c->ack_buff[i].seq_id > 0 && p->ack_seq - c->ack_buff[i].seq_id <= 32 && (p->ack_bits & 1 << (p->ack_seq - c->ack_buff[i].seq_id-1))) ) { //Remove this field
					printf("Recieved ack for packet[%d], it took: %ld millis\n", c->ack_buff[i].seq_id, s->cur_time - c->ack_buff[i].sent_t);
				
					//record rtt and update average
					c->n_acked += 1;
					c->total_rtt += s->cur_time - c->ack_buff[i].sent_t;
					c->avg_rtt = c->total_rtt / c->n_acked;

					memmove(&c->ack_buff[i], &c->ack_buff[i+1], sizeof(struct Pckt) *(ACK_BUFF_LEN-1-i));
					--c->ack_cnt;
			}
		}
	}

}

void ServTimeoutClients(struct Server *s) {
	if (!s) {
		return;
	}
		
	long res;
	
	for (int i = s->num_cli-1; i >= 0; --i) {
		res = s->cur_time - s->clients[i].last_rcv_time;
		if (res >= s->c_timeout) {
			ServSend(s, &s->clients[i], PKT_FLAG_DISC, NULL, 0);
			printf("Client [%lx] timed out (%ld milli's since last packet)!\n", s->clients[i].id, res);
			ServRemoveCli(s, s->clients[i].id);
		}
	}
}

struct CliData* ServAddCli(struct Server *s, struct sockaddr_in *addr) {
	if (!s || s->num_cli == s->max_cli) {
		return NULL;
	}
	
	struct CliData *new_c = &s->clients[s->num_cli++];
	if (!new_c) {
		return NULL; 
	}
	memset(new_c, 0, sizeof(struct CliData));

	new_c->addr = *addr;
	new_c->id = ServGenId();
	
	new_c->last_rcv_time = s->cur_time;

	return new_c;
}

struct CliData* ServGetCli(struct Server *s, long c_id) {
	if (!s || s->num_cli == 0) {
		return NULL;
	}

	for (int i=s->num_cli-1; i >= 0; --i) {
		if (s->clients[i].id == c_id) {
			return &s->clients[i];
		}
	}

	return NULL;
}

int ServRemoveCli(struct Server *s, long c_id) {
	if (!s || s->num_cli == 0) {
		return 0;
	}

	for (int i=s->num_cli-1; i >= 0; --i) {
		if (s->clients[i].id == c_id) {
			memmove(&s->clients[i], &s->clients[i+1], sizeof(struct CliData)*(s->max_cli - i -1));
			--s->num_cli;
			return 1;
		}
	}
	
	return 0;
}

long ServGenId() {
	long n;
	int *n_ptr = (int*)&n;

	n_ptr[0] = rand();
	n_ptr[1] = rand();

	return n;
}

/*CLIENT*/
struct CliConn*  CliConnect(char *addr, short portno, int timeout) {
	int _sock=0;
	struct sockaddr_in _addr;
	
	char c_recv[64];

 	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(portno);

   if (inet_aton(addr, (struct in_addr*)&_addr.sin_addr.s_addr) == 0) {
		printf("Error resolving dotted address: %s\n", addr);
		return NULL;
	}
   
	_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (_sock <= 0) {
		perror("Socket system call failed");
		return NULL;
   }

	struct CliConn *new_c;
	new_c = malloc(sizeof(struct CliConn));
	memset(new_c, 0, sizeof(struct CliConn));
	
	new_c->sock = _sock;
	new_c->addr = _addr;

	new_c->max_len = 64;
	new_c->recv_buff = c_recv;

	short vers[2];
	vers[0] = VERSION_MAJOR;
	vers[1] = VERSION_MINOR;

	int n = 0;
	if ((n=CliSend(new_c, PKT_FLAG_CONNECT, (char*)vers, 4)) > 0) {
		struct RecvPckt *p = CliRecv(new_c, timeout);
		if (p && p->flags & PKT_FLAG_CONNECT && p->size == 10) {
			//Server successfully responded to request
			new_c->id = *(long*)p->payload;
			new_c->start_time = millitime();
			new_c->cur_time = new_c->start_time;
			new_c->max_len = *(short*)(p->payload+8);
			new_c->recv_buff = malloc(new_c->max_len);
			if (!new_c) {
				printf("Error allocating buffer for networking\n");
				free(new_c);
				return NULL;
			}
			printf("Servers length: %d\n", new_c->max_len);
			new_c->ack_seq = 1;
			new_c->ack_bits = 0;
		} else {
			printf("No Response from server!\n");

			free(new_c);
			return NULL;
		}
	}

	printf("Recieved ID: %lx\n", new_c->id); 

	return new_c;
}

int CliPcktSend(struct CliConn *c, struct Pckt *p) {
	if (!c || !p) {
		return -1;
	}

	if (p->size + PACKET_HEAD_SIZE >= c->max_len) {
		printf("Payload to large for connecttion: %d of %d bytes\n", p->size + PACKET_HEAD_SIZE, c->max_len -1);
		return -1;
	}
	
	char *msg_buff;
	if (p->size > 0 && p->payload) {
			msg_buff = (char *)malloc(PACKET_HEAD_SIZE+p->size);
	} else {
			msg_buff = (char *)malloc(PACKET_HEAD_SIZE);
	}

	//fill with nummy data
	*(short*)msg_buff = 0xFEFF;
	*(char*)(msg_buff+2) = p->flags; //flags
	*(long*)(msg_buff+3) = c->id;
	*(int*)(msg_buff+11) = ++c->l_seq;
	*(int*)(msg_buff+15) = c->ack_seq;
	*(int*)(msg_buff+19) = c->ack_bits;
	*(short*)(msg_buff+23) = p->size;

	if (p->size > 0 && p->payload) {
		memcpy(msg_buff+PACKET_HEAD_SIZE, p->payload, p->size);
	}

	int n = sendto(c->sock, msg_buff, (PACKET_HEAD_SIZE+p->size), 0, (struct sockaddr*)&c->addr, sizeof(c->addr));
	if (n == p->size+PACKET_HEAD_SIZE) {
		//sent this packet
		p->sent_t = c->cur_time;
		if (c->ack_cnt < ACK_BUFF_LEN) {
			memcpy(&c->ack_buff[c->ack_cnt++], p, sizeof(struct Pckt));	
		} else {
			//drop the first packet
			printf("Dropped packet: %d\n", c->ack_buff[0].seq_id);

			memmove(c->ack_buff, &c->ack_buff[1], sizeof(struct Pckt) * (ACK_BUFF_LEN-1));
			memcpy(&c->ack_buff[ACK_BUFF_LEN-1], p, sizeof(struct Pckt));
		}

		free(msg_buff);
		return 0;
	}

	free(msg_buff);
	return errno;

}

int CliSend(struct CliConn *c, char f, char *buff, unsigned short buff_len) {
	int n=0;
	socklen_t a_len = sizeof(struct sockaddr_in);

	if(!c) {
		return -1;
	}

	if (buff_len+PACKET_HEAD_SIZE >= c->max_len) {
		printf("Payload to large for connection: %d of %d bytes\n", buff_len+PACKET_HEAD_SIZE, c->max_len-1);
		return -1;
	}

	//Create the sending buffer and copy to it
	char *msg_buff = (char*)malloc(PACKET_HEAD_SIZE + buff_len);

	//Prepare the ConnHeader portioni
	*(short*)msg_buff = 0xFEFF;
	*(char*)(msg_buff+2) = f; //flags
	*(long*)(msg_buff+3) = c->id;
	*(int*)(msg_buff+11) = ++c->l_seq;
	*(int*)(msg_buff+15) = c->ack_seq;
	*(int*)(msg_buff+19) = c->ack_bits;
	*(short*)(msg_buff+23) = buff_len;
	
	//copy message
	if (buff && buff_len > 0) {  
		memcpy(msg_buff+PACKET_HEAD_SIZE, buff, buff_len);
	}

	n = sendto(c->sock, msg_buff, (PACKET_HEAD_SIZE+buff_len), 0, (struct sockaddr*)&c->addr, a_len);
	if (buff_len+PACKET_HEAD_SIZE == n) { 
		if (c->ack_cnt == ACK_BUFF_LEN) {
			//drop the first packet
			printf("Dropped packet: %d\n", c->ack_buff[0].seq_id);

			memmove(c->ack_buff, &c->ack_buff[1], sizeof(struct Pckt) * (ACK_BUFF_LEN-1));
			c->ack_cnt--;
		}
		c->ack_buff[c->ack_cnt].sent_t = c->cur_time;
		c->ack_buff[c->ack_cnt].flags = f;
		c->ack_buff[c->ack_cnt].seq_id = c->l_seq;
		c->ack_buff[c->ack_cnt].size = buff_len;
		
		if (buff_len > 0 && buff) {
			c->ack_buff[c->ack_cnt].payload = malloc(buff_len);
			memcpy(c->ack_buff[c->ack_cnt].payload, buff, buff_len);
		} else {
			c->ack_buff[c->ack_cnt].payload = NULL;
		}

		c->ack_cnt++;

		free(msg_buff);	
		return n;
	}	

	free(msg_buff);
	return 0;
	
}

struct RecvPckt* CliRecv(struct CliConn *c, int timeout) {
	if(!c || !c->recv_buff) {
		return NULL;
	}
	
	int opts = 0;
	int n = 0;

	if (timeout <= 0) { 
		opts |= MSG_DONTWAIT;
	} else {
		setGlobalTimeout(c->sock, timeout);
	}

	struct RecvPckt *new_p = malloc(sizeof(struct RecvPckt));
	memset(new_p, 0, sizeof(struct RecvPckt));

	socklen_t r_len = sizeof(new_p->origin);	
	
	//flush recieving buffer
	memset(c->recv_buff, 0, c->max_len);

	//recieve raw packet
	n = recvfrom(c->sock, c->recv_buff, c->max_len, opts,
						 (struct sockaddr*)&new_p->origin, &r_len);
	if (n > 0 && n < c->max_len && addrcmp(&new_p->origin, &c->addr)) { 
		//if successful extract data
		printf("Recieved Packet: %d bytes\n", n);

		char *msg_buff = c->recv_buff;
		
		if (msg_buff[0] == -2 && msg_buff[1] == -1) { //proper order
			printf("Reversed byte order TODO (BIG endian)\n");
		} else if ((int)msg_buff[0] == -1 && (int)msg_buff[1] == -2) { //convert from stupid byte order
			new_p->flags = msg_buff[2];
			new_p->cli_id = *(long*)(msg_buff+3);
			new_p->seq_id = *(int*)(msg_buff+11);
			new_p->ack_seq = *(int*)(msg_buff+15);
			new_p->ack_bits = *(int*)(msg_buff+19);
			new_p->size = *(short*)(msg_buff+23);

			//extract packet
			new_p->payload = malloc(new_p->size);
			memcpy(new_p->payload, msg_buff+25, new_p->size);
		}
	   
		return new_p;
	} 

	free(new_p);
	return NULL;
}

struct ServInfo* CliQueryServ(char *addr, int timeout) {
	if (!addr) {
		return NULL;
	}

	struct ServInfo* new_info;
	struct sockaddr_in s, r_s;
	socklen_t s_len = sizeof(r_s);

	char msg_b[PACKET_HEAD_SIZE];

	int opts = 0;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("Error creating socket for query");
		return NULL;
	}

	if (!strToAddr(&s, addr)) {
		perror("Error converting string to socket");
		close(sock);
		return NULL;
	}

	*(short*)msg_b = 0xFEFF;
	*(char*)(msg_b+2) = PKT_FLAG_QUERY; //flags
	memset((msg_b+3), 0, PACKET_HEAD_SIZE-3);

	int n = sendto(sock, msg_b, PACKET_HEAD_SIZE, 
						 0, (struct sockaddr*)&s, sizeof(struct sockaddr_in));
	if (n == PACKET_HEAD_SIZE) {	
		//successfully sent now wait for reply
		if (timeout <= 0) {
			opts |= MSG_DONTWAIT;
		} else {
			setGlobalTimeout(sock, timeout);
		}
		
		char recv_buff[48];
		memset(recv_buff, 0, 48);

		n = recvfrom(sock, recv_buff, 48, opts,
							 (struct sockaddr*)&r_s, &s_len);
		if (n >= 5 && n < 48) { 
			//recieved reply from proper server
			new_info = malloc(sizeof(struct ServInfo));
			
			//memcpy(&new_info->s_addr, &r_s, sizeof(struct sockaddr_in));
			new_info->pass_protect = recv_buff[0];
			new_info->min_version = recv_buff[1];
			new_info->maj_version = recv_buff[2];

			new_info->n_players = recv_buff[3];
			new_info->max_players = recv_buff[4];
			if (recv_buff[5] > 0) {
				memcpy(&new_info->s_name, &recv_buff[6], recv_buff[5]);
				new_info->s_name[recv_buff[5]] = 0;
			} else {
				new_info->s_name[0] = 0;
			}
			
			close(sock);
			return new_info;
			
		}
	}
	
	close(sock);
	perror("Error sending query payload");
	return NULL;

}

void CliProcessAck(struct CliConn *c, struct RecvPckt *p) {
	if (!c || !p) {
		return;
	}

	c->n_recv++;

	int res = p->seq_id - c->ack_seq;
	if (res >= 1 && res <= 32) { //newer sequenced packet
		c->ack_bits = ((c->ack_bits << 1) | 1);
		c->ack_bits = (c->ack_bits << (res-1));
		c->ack_seq = p->seq_id;
	} else if (res < 0 && res >= -32) { //packet is older
		c->ack_bits |= (1 << (c->ack_seq - p->seq_id - 1));
	} else if (res > 32) {
		c->ack_bits = 0;
		c->ack_seq = p->seq_id;
	}

	printf("Processed acks: %d\n", c->ack_bits);

	if (c->ack_cnt > 0) {
		for (int i=c->ack_cnt-1; i >= 0; --i) {
			if (c->ack_buff[i].seq_id == p->ack_seq || (p->ack_seq - c->ack_buff[i].seq_id > 0 && p->ack_seq - c->ack_buff[i].seq_id <= ACK_BUFF_LEN && (p->ack_bits & 1 << (p->ack_seq - c->ack_buff[i].seq_id-1))) ) { //Remove this field
					printf("Recieved ack for packet[%d] after %ld milliseconds\n", c->ack_buff[i].seq_id, c->cur_time - c->ack_buff[i].sent_t);
					memmove(&c->ack_buff[i], &c->ack_buff[i+1], sizeof(struct Pckt) *(ACK_BUFF_LEN-1-i));
					--c->ack_cnt;
			}
		}
	}
}
