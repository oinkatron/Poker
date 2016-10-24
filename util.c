#include "util.h"

int addrToStr(char *buff, struct sockaddr_in *a, size_t buff_len) {
	int n=0;
	if ((n=snprintf(buff, buff_len, "%s:%d", inet_ntoa(a->sin_addr), ntohs(a->sin_port))) > 0) {
		return n;
	}

	return -1;
}

int strToAddr(struct sockaddr_in *a, const char *buff) {
	if (!a || !buff) {
		return 0;
	}

	char *pos;
	if ((pos=strstr(buff, ":"))) {
		char ip_buff[pos-buff+1];
		memcpy(ip_buff, buff, pos-buff);
		ip_buff[pos-buff] = 0;

		if (!inet_pton(AF_INET, ip_buff, &a->sin_addr)) {
			fprintf(stderr, "Error: Canot convert dotted addr: %s\n", buff);
			return 0;
		}
	
		a->sin_port = htons(atoi(&pos[1]));
		a->sin_family = AF_INET;

	} else {
		fprintf(stderr, "Error: Invalid address: [%s], must specify port with ':'\n", buff);
		return 0;
	}

	return 1;
}

int addrcmp(struct sockaddr_in *a, struct sockaddr_in *b) {
	if (a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port) {
		return 1;
	}

	return 0;
}

void setGlobalTimeout(int fd, unsigned long t) {
	struct timeval tim;
	
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		perror("Cannot retrieve file descriptor flags!");
		return;
	}

	if (t == 0) {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	} else {
		tim.tv_sec = t / 1000;
		tim.tv_usec = 1000 * (t % 1000);

		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
		if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tim, sizeof(tim)) < 0) {
			fprintf(stderr, "Error: Cannot set receiving timeout\n");
		}
	
		if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&tim, sizeof(tim)) < 0) {
			fprintf(stderr, "Error: Cannot set sending timeout\n");
		}
	}
}

void setSendTimeout(int fd, unsigned long t) {
	struct timeval tim;

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		perror("Cannot retrieve file descriptor flags!");
		return;
	}
	
	if (t == 0) {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);	
	} else {
		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
		tim.tv_sec = t / 1000;
		tim.tv_usec = 1000 * (t % 1000);
	
		if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&tim, sizeof(tim)) < 0) {
			fprintf(stderr, "Error: setting recieving timeout!\n");
		}
	}

}

void setRecvTimeout(int fd, unsigned long t) {
	struct timeval tim;
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		perror("Cannot retrieve file descriptor flags!");
		return;
	}
	
	if (t == 0) {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);	
	} else {
		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
		tim.tv_sec = t / 1000;
		tim.tv_usec = 1000 * (t % 1000);
	
		if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&tim, sizeof(tim)) < 0) {
			printf("Error setting recieving timeout!\n");
		}
	}
}

unsigned long millitime() {
	struct timeval t;
	gettimeofday(&t, NULL);

	return (long)((t.tv_sec * 1000) + (t.tv_usec * 0.001));
}

void millisleep(long milli) {
	struct timespec req, rem;

	req.tv_sec = milli / 1000;
	req.tv_nsec = 100000 * (milli % 1000);


	int nn=0;
	if ((nn=nanosleep(&req, &rem)) < 0) {
		if (errno == EINTR) { 
			nanosleep(&rem, &req);
		} else {
			perror("Error sleeping for requested time");
		}
	}
}
