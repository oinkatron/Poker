#ifndef UTIL_H 
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <string.h>
#include <errno.h>

#include <openssl/hmac.h>
#include <openssl/evp.h>


#define NEW(d) malloc(sizeof(d));
#define ZERO(p,t) memset((p), 0, sizeof(t))
#define ARRAY(d,s) malloc(sizeof(d)*(s))

int addrToStr(char *buff, struct sockaddr_in *a, size_t buff_len);
int strToAddr(struct sockaddr_in *a, const char *buff);

int addrcmp(struct sockaddr_in *a, struct sockaddr_in *b);

void setGlobalTimeout(int fd, unsigned long t);
void setSendTimeout(int fd, unsigned long t);
void setRecvTimeout(int fd, unsigned long t);

#define CHALLENGE_SIZE 16

unsigned char *generateChallenge(char *k, char *m); //generates digest of key and generated secret filled into m (m must be at least CHALLENGE_SIZE big
int 	       answerChallenge();

unsigned long millitime();
void millisleep(long milli);

//Hashing funcs


#endif
