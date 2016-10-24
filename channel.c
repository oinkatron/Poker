#include "channel.h"
#include "util.h"

#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <strings.h>

#define PCKT_SET_BOM(mb,d) *(uint16_t*)(mb) = (d)
#define PCKT_GET_BOM(mb,d) (d) = *(uint8_t*)(mb)

#define PCKT_SET_FLAGS(mb,d) *(uint8_t*)(mb+2) = (d)
#define PCKT_GET_FLAGS(mb,d) (d) = *(uint8_t*)(mb+2)

#define PCKT_SET_SEQID(mb,d) *(uint32_t*)(mb+3) = (d)
#define PCKT_GET_SEQID(mb,d) (d) = *(uint32_t*)(mb+3)

#define PCKT_SET_ACKID(mb,d) *(uint32_t*)(mb+7) = (d)
#define PCKT_GET_ACKID(mb,d) (d) = *(uint32_t*)(mb+7)

#define PCKT_SET_ACKBITS(mb,d) *(uint32_t*)(mb+11) = (d)
#define PCKT_GET_ACKBITS(mb,d) (d) = *(uint32_t*)(mb+11)

#define PCKT_SET_PSIZE(mb,d) *(uint16_t*)(mb+15) = (d)
#define PCKT_GET_PSIZE(mb,d) (d) = *(uint16_t*)(mb+15)

#define PCKT_SET_PAYLOAD(mb,d,s) memcpy(mb+17, d, s)
#define PCKT_GET_PAYLOAD(mb,d) (d) = *(char*)(mb+17)

struct HostData* Host_Create(struct sockaddr_in *s) {
	if (!s) {
		return NULL;
	}

	struct HostData *new_hd = NEW(struct HostData);
	ZERO(new_hd, struct HostData);
	memcpy(&new_hd->r_addr, s, sizeof(struct sockaddr_in));

	return new_hd;	
}
void Host_Free(struct HostData *hd) {
	if (hd) {
		free(hd);
	}
}

struct HostBuff* HBuff_Create(uint16_t len) {
	if (len == 0) {
		return NULL;
	}

	struct HostBuff *new_hb = NEW(struct HostBuff);
	if (!new_hb) {
		perror("malloc()");
		return NULL;
	}

	new_hb->hosts = ARRAY(struct HostData,len);
	if (!new_hb->hosts) {
		perror("ARRAY()");
		free(new_hb);
		return NULL;
	}

	new_hb->size = 0;
	new_hb->cap = len;

	return new_hb;
}

void HBuff_Init(struct HostBuff *new_hb, uint16_t len) {
	if (len == 0 || !new_hb) {
		return;
	}
	new_hb->hosts = ARRAY(struct HostData,len);
	if (!new_hb->hosts) {
		perror("ARRAY()");
		new_hb->size = 0;
		new_hb->cap = 0;
		return;
	}

	new_hb->size = 0;
	new_hb->cap = len;
}

void HBuff_Free(struct HostBuff* hb) {
	if (!hb) {
		return;
	}

	free(hb->hosts);
	free(hb);
}

struct HostData * HBuff_Push(struct HostBuff *hb, struct HostData *hd) {
	if (!hb || hb->size == hb->cap) {
		return NULL;
	}
	
	memcpy(&hb->hosts[hb->size], hd, sizeof(struct HostData));
	return &hb->hosts[hb->size++];

}

struct HostData* HBuff_Get(struct HostBuff *hb, struct sockaddr_in *ind) {
	if (!hb || !ind) {
		return NULL;
	}

	for (int i=0; i<hb->size; i++) {
		if (addrcmp(&hb->hosts[i].r_addr, ind)) {
			return &hb->hosts[i];
		}
	}

	return NULL;

}
int32_t HBuff_Remove(struct HostBuff *hb, struct sockaddr_in *ind) {
	if (!hb || !ind) {
		return 0;
	}

	for (int i=0; i<hb->size; i++) {
		if (addrcmp(&hb->hosts[i].r_addr, ind)) {
			memmove(&hb->hosts[i], &hb->hosts[i+1], (hb->size - i - 1) * sizeof(struct HostData));
			hb->size--;
			return 1;
		}
	}

	return 0;
}

int32_t HBuff_RemoveByIndex(struct HostBuff *hb, int16_t ind) {
	if (hb && ind < hb->size && ind >= 0) {
		memmove(&hb->hosts[ind], &hb->hosts[ind+1], (hb->size - ind - 1) * sizeof(struct HostData));
		--hb->size;

		return 1;
	}

	return 0;
}

struct Channel* Channel_Open(const char *addr_str, uint16_t max_hosts, uint16_t mbuff_len, uint16_t to) {
	if (!addr_str || max_hosts == 0) {
		return NULL;
	}

	struct Channel *ch = NEW(struct Channel);
	if (!ch) {
		perror("Channel_Open->malloc\n");
		return NULL;
	}

	//convert address
	if (!strToAddr(&ch->l_addr, addr_str)) {
		perror("Channel_Open->strToAddr\n");
		free(ch);
		return NULL;
	}

	ch->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (ch->sock <= 0) {
		perror("Error creating socket");
		free(ch);
		return NULL;
	}

	if (bind(ch->sock, (struct sockaddr*)&ch->l_addr, sizeof(ch->l_addr)) < 0) {
		perror("Error binding socket");
		free(ch);
		return NULL;
	}

	printf("Opened channel on: %s\n", addr_str);

	HBuff_Init(&ch->hosts, max_hosts);
	HBuff_Init(&ch->pending, 5);

	ch->max_msg_size = mbuff_len;
	ch->send_buff = malloc(mbuff_len+1);
	memset(ch->send_buff, 0, mbuff_len+1);
	ch->send_size = 0;
	ch->recv_buff = malloc(mbuff_len+1);
	memset(ch->recv_buff, 0, mbuff_len+1);
	ch->recv_size = 0;
	

	ch->host_timeout = to;
	ch->start_time = millitime();

	return ch;
}

void Channel_Close(struct Channel *ch) {

}

void Channel_Connect(struct Channel *ch, const char *addr_str, void (*cb)(struct Channel*, int16_t h_id ,int8_t)) {
	if (!ch || !addr_str) {
		return;
	}

	struct sockaddr_in r_addr;
	ZERO(&r_addr, struct sockaddr_in);
	
	//resolve addres
	if (!strToAddr(&r_addr, addr_str)) {
		return;
	}

	int32_t p_id = Channel_CreatePending(ch, &r_addr);

	if (p_id >= 0) {
		struct HostData *hd = &ch->pending.hosts[p_id];
		char data[2];

		data[0] = MAJOR_VERSION;
		data[1] = MINOR_VERSION;
		
		//send it to new host
		int n = Channel_Send(ch, hd, PCKT_FLAG_CONNECT, data, 2);
		if (n != PCKT_HEAD_SIZE+2) {
			perror("Cannot initiate connection: sendto()");
			Channel_RemovePending(ch, p_id);
		} else {
			hd->c_callback = cb;
		}
	} else {
		fprintf(stderr, "Error creating new pending host\n");
	}
}

void Channel_Disconnect(struct Channel *ch, int16_t h_id) {
	if (!ch) {
		return;
	}
	
	//Send a disconnect packet and remove host 
	//we only care that the DC pckt is sent
	if (h_id < ch->hosts.size && h_id >= 0) {
			if (!Channel_Send(ch, &ch->hosts.hosts[h_id], PCKT_FLAG_DISCONNECT, NULL, 0)) {
			fprintf(stderr, "Error sending disconnect to: %s:%d\n", inet_ntoa(ch->hosts.hosts[h_id].r_addr.sin_addr), ntohs(ch->hosts.hosts[h_id].r_addr.sin_port));
		}
	
		if (ch->on_disconnect)
			(*ch->on_disconnect)(ch, h_id);

		//Remove Host
		Channel_RemoveHost(ch, h_id);
	}
	
}

int32_t Channel_QueryHost(struct Channel *ch, const char *addr_str, char q_type) {
	if (!ch || !addr_str) {
		return 0;
	}

	struct sockaddr_in r_addr;
	if (!strToAddr(&r_addr, addr_str)) {
		return 0;
	}

	//send a query pckt
	if (Channel_SendRaw(ch, &r_addr, PCKT_FLAG_QUERY, &q_type, 1) >= 0) {
		return 1;
	}

	return 0;
}

int32_t Channel_Recv(struct Channel *ch, int timeout) {
	if (!ch) {
		return 0;
	}

	//setup delay
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = 0;
	
	if (timeout > 0) {
		delay.tv_sec = timeout / 1000;
		delay.tv_usec = timeout % 1000;
	}

	//select to see if message is waiting
	struct fd_set rd;
	FD_ZERO(&rd);
	FD_SET(ch->sock, &rd);

	int ready = select(ch->sock+1, &rd, NULL, NULL, (timeout < 0) ? NULL : &delay);

	if (ready > 0 && FD_ISSET(ch->sock, &rd)) {
		//There is a packet waiting
		struct sockaddr_in r_addr;
		socklen_t a_len = sizeof(r_addr);
		
		struct RecvPckt new_p;
		struct HostData *hd;

		int n = recvfrom(ch->sock, ch->recv_buff, ch->max_msg_size+1, 0, (struct sockaddr*)&r_addr, &a_len);	
		if (n >= PCKT_HEAD_SIZE && n <= ch->max_msg_size) {
			if (ch->recv_buff[0] == -2 && ch->recv_buff[1] == -1) { //improper order convert
				printf("Reversed byte order TODO (BIG endian)\n");
			
			} else if ((int)ch->recv_buff[0] == -1 && (int)ch->recv_buff[1] == -2) {		
				printf("Packet Recieved: flags = %x\n", ch->recv_buff[2]);
				//Extract Packet
				new_p.flags = ch->recv_buff[2];
				new_p.seq_id = *(uint32_t*)(ch->recv_buff+3);
				new_p.ack_seq = *(uint32_t*)(ch->recv_buff+7);
				new_p.ack_bits = *(uint32_t*)(ch->recv_buff+11);
				new_p.size = *(uint16_t*)(ch->recv_buff+15);
				new_p.payload = ch->recv_buff+17;

				memcpy(&new_p.origin, &r_addr, sizeof(struct sockaddr_in));
				//Check Pckt type
				if (ch->recv_buff[2] & PCKT_FLAG_CONNECT) {
					//Prepare Conn ACK
					int16_t host_id;

					//If this host is already connected
					if (HBuff_Get(&ch->hosts, &new_p.origin)) {
						fprintf(stderr, "Host: %s:%d is already connected\n", inet_ntoa(new_p.origin.sin_addr), ntohs(new_p.origin.sin_port));
						
						//Rejected client, let them know
						memset(ch->send_buff, 0, PCKT_HEAD_SIZE+2);
						
						*(ch->recv_buff+PCKT_HEAD_SIZE+1) = 1; //Already Connected
						PCKT_SET_BOM(ch->send_buff, 0xFEFF);
						PCKT_SET_FLAGS(ch->send_buff, PCKT_FLAG_CONNACK);

						int n = sendto(ch->sock, ch->send_buff, PCKT_HEAD_SIZE+2, 0,
											 (struct sockaddr*)&new_p.origin, sizeof(new_p.origin));
						if (n <= 0) {
							//Couldnt send rejection
							perror("Channel_Recv()->sendto()");
						}	

						if (ch->on_reject) {
							(*ch->on_reject)(ch, &new_p);
						}

					} else if (new_p.size < 2 || new_p.payload[0] != MAJOR_VERSION || new_p.payload[1] != MINOR_VERSION) {
						fprintf(stderr, "Host: %s:%d is not the proper version: %d.%d : %d.%d [%d bytes]\n", inet_ntoa(new_p.origin.sin_addr), ntohs(new_p.origin.sin_port), MAJOR_VERSION, MINOR_VERSION, new_p.payload[0], new_p.payload[1], new_p.size);

						//Rejected client, let them know
						memset(ch->send_buff, 0, PCKT_HEAD_SIZE+2);
						
						*(ch->send_buff+PCKT_HEAD_SIZE+1) = 3; //improper version
						PCKT_SET_BOM(ch->send_buff, 0xFEFF);
						PCKT_SET_FLAGS(ch->send_buff, PCKT_FLAG_CONNACK);

						int n = sendto(ch->sock, ch->send_buff, PCKT_HEAD_SIZE+2, 0,
											 (struct sockaddr*)&new_p.origin, sizeof(new_p.origin));
						if (n <= 0) {
							//Couldnt send rejection
							perror("Channel_Recv()->sendto()");
						}	
							  
						if (ch->on_reject)
							(*ch->on_reject)(ch, &new_p);

					} else if ((host_id=Channel_CreateHost(ch, &new_p.origin)) >= 0) {
						//Accepted Connection
						char c_ack[3];
						c_ack[0] = 1;
						
						*(uint16_t*)(c_ack+1) = ch->host_timeout;
						printf("Sent timeout: %d\n", *(uint16_t*)(c_ack+1));
						ch->hosts.hosts[host_id].timeout_t = ch->host_timeout;
						ch->hosts.hosts[host_id].last_rcv_t = ch->cur_time;
						int nn = Channel_Send(ch, &ch->hosts.hosts[host_id], PCKT_FLAG_CONNACK, c_ack, 3);
						if (n > 0) {
							//callback
							if (ch->on_accept)
								(*ch->on_accept)(ch, &new_p);
						} else {
							fprintf(stderr, "Channel_Recv()->Channel_Send()\n");
						} 
					} else {
						//Rejected client, let them know
						memset(ch->send_buff, 0, PCKT_HEAD_SIZE+2);

						*(ch->send_buff+PCKT_HEAD_SIZE+1) = 2; //Already Connected
						PCKT_SET_BOM(ch->send_buff, 0xFEFF);
						PCKT_SET_FLAGS(ch->send_buff, PCKT_FLAG_CONNACK);

						int n = sendto(ch->sock, ch->send_buff, PCKT_HEAD_SIZE+2, 0,
							(struct sockaddr*)&new_p.origin, sizeof(new_p.origin));
						if (n <= 0) {
							//Couldnt send rejection
							perror("Channel_Recv()->sendto()");
						}

						if (ch->on_reject)
							(*ch->on_reject)(ch, &new_p);
					}
				} else if (ch->recv_buff[2] & PCKT_FLAG_CONNACK) {
					if (new_p.size == 3 && new_p.payload[0] == 1) {
						int16_t p_id = Channel_GetPending(ch, &new_p.origin);
						if (p_id >= 0) {
							p_id = Channel_ConfirmPending(ch, p_id);
							ch->hosts.hosts[p_id].timeout_t = *(uint16_t*)(ch->recv_buff+PCKT_HEAD_SIZE+1);
							ch->hosts.hosts[p_id].last_rcv_t = ch->cur_time;
		
							if (ch->hosts.hosts[p_id].c_callback)
								(*ch->hosts.hosts[p_id].c_callback)(ch, p_id, 1);
						}
					} else { //Server Rejected Us
						int16_t p_id = Channel_GetPending(ch, &new_p.origin);
						if (p_id >= 0) {
							if (ch->hosts.hosts[p_id].c_callback)
								(*ch->hosts.hosts[p_id].c_callback)(ch, p_id, 0);
							
							switch(*(new_p.payload+PCKT_HEAD_SIZE+1)) {
								case 1:
									fprintf(stderr, "Host Rejected Connection: Already Connected\n");
									break;
								case 2:
									fprintf(stderr, "Host Rejected Connection: Server Full\n");
									break;
								case 3:
									fprintf(stderr, "Version Mismatch!\n");
									break;
							}
							Channel_RemovePending(ch, p_id);
						}
					}
				} else if (ch->recv_buff[2] & PCKT_FLAG_QUERY) {
					if (ch->on_query)
						(*ch->on_query)(ch, &new_p);
				} else if (ch->recv_buff[2] & PCKT_FLAG_QUERYACK) {
					if (ch->on_query_response)
						(*ch->on_query_response)(ch, &new_p);
				} else {
					//This is a message from a connected host
					int16_t p_id = Channel_GetHost(ch, &r_addr);
					if (p_id >= 0) {
						//Process Acks
						Channel_ProcessPckt(ch, &ch->hosts.hosts[p_id], &new_p);

						if (ch->recv_buff[2] & PCKT_FLAG_MSG) {
							if (ch->on_msg)
								(*ch->on_msg)(ch, p_id, &new_p);
						} else if (ch->recv_buff[2] & PCKT_FLAG_DISCONNECT) {
							//This host is done, delete data
							if (ch->on_disconnect)
								(*ch->on_disconnect)(ch, p_id);

							Channel_RemoveHost(ch, p_id);
						} else if (ch->recv_buff[2] & PCKT_FLAG_PING) {
							if (ch->on_ping) {
								(*ch->on_ping)(ch, p_id);
							}
						}
					}
				}

				//Log and Free Packet
				char adr_str[24];
				addrToStr(adr_str, &r_addr, 24);
				//free(new_p);
				return 1;
			}
		}
	} else if (ready < 0) {
		perror("select()");
	}

	return 0;
}

int32_t Channel_SendRaw(struct Channel *ch, struct sockaddr_in *ra, char flags, char *msg, int16_t msg_len) {
	if (!ch || !ra) {
		return -1;
	}

	if (msg_len+PCKT_HEAD_SIZE >= ch->max_msg_size) {
		return -1;
	}

	//clear send buffer
	memset(ch->send_buff, 0, ch->max_msg_size+1);

	PCKT_SET_BOM(ch->send_buff, 0xFEFF);
	PCKT_SET_FLAGS(ch->send_buff, flags);

	if (msg_len <= 0 || !msg) {
		PCKT_SET_PSIZE(ch->send_buff, 0);
	} else {
		PCKT_SET_PSIZE(ch->send_buff, msg_len);
		PCKT_SET_PAYLOAD(ch->send_buff, msg, msg_len);
	}

	int32_t n = sendto(ch->sock, ch->send_buff, msg_len+PCKT_HEAD_SIZE, 0,
						 	(struct sockaddr*)ra, sizeof(struct sockaddr_in));
	if (n == PCKT_HEAD_SIZE+msg_len) {
		//just return nothing else to be done
		return n;
	} else {
		fprintf(stderr, "Channel_SendRaw()->sendto");
	}

	return -1;
}

int32_t Channel_Send(struct Channel *ch, struct HostData *hd, uint8_t f, const char *buff, uint16_t buff_len) {
	if (!ch || !hd) {
		return 0;
	}
	
	if (buff_len+PCKT_HEAD_SIZE > ch->max_msg_size) {
		fprintf(stderr, "Payload to large for channel: %d of %d bytes\n", buff_len+PCKT_HEAD_SIZE, ch->max_msg_size);
		return -1;
	}

	int n=0;

	//Prepare the ConnHeader portioni
	*(int16_t*)ch->send_buff = 0xFEFF;
	*(uint8_t*)(ch->send_buff+2) = f; //flags
	*(uint32_t*)(ch->send_buff+3) = ++hd->l_seq;
	*(uint32_t*)(ch->send_buff+7) = hd->ack_seq;
	*(uint32_t*)(ch->send_buff+11) = hd->ack_bits;
	*(uint16_t*)(ch->send_buff+15) = buff_len;
		
	//copy message
	if (buff && buff_len > 0) {  
		memcpy(ch->send_buff+PCKT_HEAD_SIZE, buff, buff_len);
	} else {
		*(uint16_t*)(ch->send_buff+15) = 0;
		buff_len = 0;
	}

	n = sendto(ch->sock, ch->send_buff, (PCKT_HEAD_SIZE+buff_len), 0, (struct sockaddr*)&hd->r_addr, sizeof(struct sockaddr_in));
	if (buff_len+PCKT_HEAD_SIZE == n) {
		//successfully sent
		hd->n_sent++;
		hd->last_snt_t = ch->cur_time;

		//copy payload and flags to pckt struct
		if (hd->ack_cnt < ACK_BUFF_LEN) {
			hd->ack_buff[hd->ack_cnt].flags = f;
			hd->ack_buff[hd->ack_cnt].sent_t = ch->cur_time;
			hd->ack_buff[hd->ack_cnt].seq_id = hd->l_seq;
			hd->ack_buff[hd->ack_cnt].size = buff_len;

			if (buff_len > 0  && buff) {
				hd->ack_buff[hd->ack_cnt].payload = malloc(buff_len);
				memcpy(hd->ack_buff[hd->ack_cnt].payload, buff, buff_len);
			} else {
				hd->ack_buff[hd->ack_cnt].payload = NULL;
			}

			++hd->ack_cnt;

		} else {
			//drop this pckt
			fprintf(stderr, "Channel_Send(): Packet [%d] dropped!\n", hd->ack_buff[0].seq_id);
			if (ch->on_drop) 
				(*ch->on_drop)(ch, Channel_GetHost(ch, &hd->r_addr), &hd->ack_buff[0]);

			memmove(hd->ack_buff, &hd->ack_buff[1], sizeof(struct Pckt) * (ACK_BUFF_LEN-1));
			
			//Create new Pckt from sent data
			hd->ack_buff[hd->ack_cnt-1].flags = f;
			hd->ack_buff[hd->ack_cnt-1].sent_t = ch->cur_time;
			hd->ack_buff[hd->ack_cnt-1].seq_id = hd->l_seq;
			hd->ack_buff[hd->ack_cnt-1].size = buff_len;
			
			//copy packet contents onto HostData's payload
			//TODO: OPTIMIZE 
			if (buff_len > 0 && buff) {
				hd->ack_buff[hd->ack_cnt-1].payload = malloc(buff_len);
				memcpy(hd->ack_buff[hd->ack_cnt-1].payload, buff, buff_len);
			} else {
				hd->ack_buff[hd->ack_cnt-1].payload = NULL;
			}

		}

		return n;

	} else {
		perror("Channel_Send->sendto");
		return 0;
	}

	return 0;
}

void Channel_ProcessPckt(struct Channel *ch, struct HostData *hd, struct RecvPckt *p) {
	if (!hd || !p || !ch) {
		return;
	}

	hd->n_recvd++;
	hd->last_rcv_t = ch->cur_time;

	//push packets id onto ack buffer to send next time
	int res = p->seq_id - hd->ack_seq;
	if (res >= 1 && res <= 32) { //newer sequenced packet
		hd->ack_bits = (hd->ack_bits << 1) | 1;
		hd->ack_bits = (hd->ack_bits << (res-1));
		hd->ack_seq = p->seq_id;
	} else if (res < 0 && res >= -32) { //packet is older
		printf("Older packet recieved!");
		hd->ack_bits |= 1 << (hd->ack_seq - p->seq_id - 1);
	} else if (res > 32) {
		hd->ack_bits = 0;
		hd->ack_seq = p->seq_id;
	}

	//check recieved ack against unacked packets
	if (hd->ack_cnt > 0) {
		for (int i=hd->ack_cnt-1; i >= 0; --i) {
			if (hd->ack_buff[i].seq_id == p->ack_seq || (p->ack_seq - hd->ack_buff[i].seq_id > 0 && p->ack_seq - hd->ack_buff[i].seq_id <= 32 && (p->ack_bits & 1 << (p->ack_seq - hd->ack_buff[i].seq_id-1))) ) { //Remove this field
					printf("Recieved ack for packet[%d], it took: %ld millis\n", hd->ack_buff[i].seq_id, ch->cur_time - hd->ack_buff[i].sent_t);
				
					//record rtt and update average
					hd->n_acked += 1;
					hd->total_rtt += ch->cur_time - hd->ack_buff[i].sent_t;
					hd->avg_rtt = hd->total_rtt / hd->n_acked;

					memmove(&hd->ack_buff[i], &hd->ack_buff[i+1], sizeof(struct Pckt) *(ACK_BUFF_LEN-1-i));
					--hd->ack_cnt;
			}
		}
	}

}

int16_t Channel_CreatePending(struct Channel *ch, struct sockaddr_in *addr) {
	if(!ch || !addr) {
		return -1;
	}

	if (ch->pending.size < ch->pending.cap) {
		ZERO(&ch->pending.hosts[ch->pending.size], struct HostData);

		memcpy(&ch->pending.hosts[ch->pending.size].r_addr, addr, sizeof(struct sockaddr_in)); 

		return ch->pending.size++;
	}

	return -1;
}

int16_t Channel_CreateHost(struct Channel *ch, struct sockaddr_in *addr) {
	if(!ch || !addr) {
		return -1;
	}

	if (ch->hosts.size < ch->hosts.cap) {
		printf("Creating Host %d of %d\n", ch->hosts.size+1, ch->hosts.cap);
		ZERO(&ch->hosts.hosts[ch->hosts.size], struct HostData);

		memcpy(&ch->hosts.hosts[ch->hosts.size].r_addr, addr, sizeof(struct sockaddr_in)); 

		return ch->hosts.size++;
	}

	return -1;

}

void Channel_RemovePending(struct Channel *ch, int16_t ind) {
	if(ch) {
		HBuff_RemoveByIndex(&ch->pending, ind);
	}
}

void Channel_RemoveHost(struct Channel *ch, int16_t ind) {
	if(ch) {
		HBuff_RemoveByIndex(&ch->hosts, ind);
	}

}

int16_t Channel_ConfirmPending(struct Channel *ch, int16_t ind) {
	if (!ch || ind < 0 || ind >= ch->pending.size) {
		return -1;
	}

	if (HBuff_Push(&ch->hosts, &ch->pending.hosts[ind])) {
		HBuff_RemoveByIndex(&ch->pending, ind);
		return ch->hosts.size-1;
	}

	return -1;
}

int16_t Channel_GetPending(struct Channel *ch, struct sockaddr_in *addr) {
	if (!ch || !addr) {
		return -1;
	}

	for (int i=0; i<ch->pending.size; ++i) {
		if (addrcmp(&ch->pending.hosts[i].r_addr, addr)) {
			return i;
		}
	}

	return -1;
}

int16_t Channel_GetHost(struct Channel *ch, struct sockaddr_in *addr) {
	if (!ch || !addr) {
		return -1;
	}

	for (int i=0; i<ch->hosts.size; ++i) {
		if (addrcmp(&ch->hosts.hosts[i].r_addr, addr)) {
			return i;
		}
	}

	return -1;
}

void Channel_Update(struct Channel *ch, uint64_t ct) {
	if (!ch) {
		return;
	}

	//update channel time
	ch->cur_time = ct;

	struct HostData *hd = NULL;
	//loop through and dc any connections which havent sent a packet
	//ping any which havent repsonded in a while
	for (int i=ch->hosts.size-1; i >= 0; --i) {
		hd = &ch->hosts.hosts[i];
		
		//printf("Checking host with id: %d and timeout: %d, %ld\n", i, hd->timeout_t, ch->cur_time - hd->last_rcv_t);
		if (hd->timeout_t < ch->cur_time - hd->last_rcv_t) {
			//This host has timed out
			printf("Disconnected\n");
			Channel_Disconnect(ch, i);		
		} 
		
		if (hd->timeout_t / 2 < ch->cur_time - hd->last_snt_t) {
			//We havent sent a message in a while better ping
			Channel_Send(ch, hd, PCKT_FLAG_PING, NULL, 0);
		}
	}
}

void Channel_Quit(struct Channel *ch, int q_code) {
	if (!ch) {
		//You fucked er bud
		exit(-1);
	}

	for (int i=ch->hosts.size-1; i >= 0; --i) {
		Channel_Disconnect(ch, i);
	}

	if (ch->on_quit) {
		(*ch->on_quit)(ch);
	}

	exit(q_code);
}

