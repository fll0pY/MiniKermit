#include "lib.h"

// Adds the header in an empty message
void addHeader(msg* message, char type, int dataLen, int seq) {
    KermitHeader *header = malloc(sizeof(KermitHeader));

    header->type = type;
    header->soh = SOH;
    header->seq = seq;
    
    header->len = sizeof(KermitHeader) + dataLen + sizeof(KermitTrailer) - 2;
    memcpy(message->payload, header, sizeof(KermitHeader));
    message->len = sizeof(KermitHeader);
}

// Adds the trailer (crc + MARK)
void addTrailer(msg* message) {
    KermitTrailer trailer;

    trailer.crc = crc16_ccitt(message->payload, message->len);
    trailer.mark = MARK;

    memcpy(message->payload + message->len, &trailer, sizeof(trailer));
    message->len += sizeof(KermitTrailer);
}

// Incapsulates a kermit message in payload
void packMessage(msg* message, char type, char* data, int dataLen, int seq) {
    addHeader(message, type, dataLen, seq);

    // Add data
    memcpy(message->payload + message->len, data, dataLen);
    message->len += dataLen;

    addTrailer(message);
}

// Creates the data filed for a Send-Init message
void packInitData(char *buff) {
    InitData data;

    data.maxl = MAXL;
    data.time = MAX_TIMEOUT / 1000;
    data.npad = 0;
    data.padc = 0;
    data.eol = MARK;
    data.qctl = 0;
    data.qbin = 0;
    data.chkt = 0;
    data.rept = 0;
    data.capa = 0;
    data.r = 0;

    memcpy(buff, &data, sizeof(InitData));
}

// Extracts the header
void unpackHeader(const msg* message, KermitHeader* header) {
	memcpy(header, message->payload, sizeof(KermitHeader));
}

// Extracts the trailer
void unpackTrailer(const msg* message, KermitTrailer* trailer) {
	memcpy(trailer, message->payload + message->len - sizeof(KermitTrailer),
													 sizeof(KermitTrailer));
}

// Calculates the crc sum for a message
unsigned short getCRC(msg* message) {
	return crc16_ccitt(message->payload, message->len - sizeof(KermitTrailer));
}

// Extracts data field from a message
int unpackData(const msg* message, void* data) {
	KermitHeader header;
    unpackHeader(message, &header);

    unsigned char dataLen = header.len - sizeof(KermitTrailer) - 2;

    memcpy(data, message->payload + sizeof(KermitHeader), dataLen);

    return dataLen;
}

// Increments seq mod MAX_SEQ
int incSeq(int seq) {
	seq++;
	return seq==MAX_SEQ?0:seq;
}

// Checks crc sum for a message
int checkCRC(msg* message) {
    KermitTrailer trailer;
    unpackTrailer(message, &trailer);

    unsigned short extractedCRC = trailer.crc; 
    unsigned short crc = getCRC(message);

    return extractedCRC == crc;
}