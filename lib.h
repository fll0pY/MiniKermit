#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef LIB
#define LIB

typedef struct {
    int len;
    char payload[1400];
} msg;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#define SEND_INIT 'S'
#define FILE_HEADER 'F'
#define DATE 'D'
#define EoF 'Z'
#define EOT 'B'
#define ACK 'Y'
#define NAK 'N'
#define ERROR 'E'

#define MAX_RETRIES 3
#define MAX_TIMEOUT 5000
#define MAXL 250
#define MAX_SEQ 64

#define SOH 0x01
#define MARK 0x0D

typedef struct {
	char soh;
	char len;
	char seq;
	char type;
} KermitHeader;

typedef struct {
	unsigned short crc;
	char mark;
} KermitTrailer;

typedef struct {
	char maxl;
	char time;
	char npad;
	char padc;
	char eol;
	char qctl;
	char qbin;
	char chkt;
	char rept;
	char capa;
	char r;
} InitData;

void addHeader(msg* message, char type, int dataLen, int seq);
void addTrailer(msg* message);

void packInitData(char *buff);
void packMessage(msg* message, char type, char* data, int dataLen, int seq);

void unpackHeader(const msg* message, KermitHeader* header);
void unpackTrailer(const msg* message, KermitTrailer* trailer);
int unpackData(const msg* message, void* data);

unsigned short getCRC(msg* message);
int checkCRC(msg* message);
int incSeq(int seq);

#endif