#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

void checkError(int result) {
	if (result == 0) {
		printf("Ksender: Link error!\n");
		fflush(stdout);
		exit(-1);
	}
}

// Increments the seq field from a message
void incMessageSeq(msg* message, int count) {
	KermitHeader header;
	unpackHeader(message, &header);

	for (int i = 0; i < count; i++) {
		header.seq = incSeq(header.seq);
	}

	memcpy(message->payload, &header, sizeof(KermitHeader));
}

// Sends a message and waits for ACK or NAK
// Returns 1 -> received ACK, -1 -> received NAK, 0 -> received nothing after 3 retries
int sendPacket(msg* message, int seq) {
	KermitHeader header;
	msg* receive;

	int retries = 0;
	int received = 0;
	while (retries < MAX_RETRIES && !received) {
		send_message(message);

		// Receive ACK OR NAK
		receive = receive_message_timeout(MAX_TIMEOUT);
		if (receive != NULL) {
			
			unpackHeader(receive, &header);
			if (header.seq == seq && (header.type == ACK || header.type == NAK)) {
				received = 1;
			}
			else {
				// Discards the packet
				retries--;
			}
		}
		retries++;
	}

	if (received) {
		if (header.type == ACK) {
			// If ACK
			return 1;
		}
		// If NAK
		return -1;
	}

	// Timeout for 3 retries -> Link error
	return 0;
}

// Sends a message until we it receives ACK
int sendUntilACK(msg* message, int seq) {
	int rez = -1;

	while (rez == -1) {
		seq = incSeq(seq);
		rez = sendPacket(message, seq);
		seq = incSeq(seq);
		incMessageSeq(message, 2);
		checkError(rez);
	}

	return seq;
}

// Sends the Send-Init message
int sendInit(int seq) {
	char buff[sizeof(InitData)];
	msg message;

	packInitData(buff);
	packMessage(&message, SEND_INIT, buff, sizeof(InitData), seq);
	seq = sendUntilACK(&message, seq);

	return seq;
}

// Sends the file name in a message
int sendFileHeader(int seq, char* fileName) {
	msg message;

	packMessage(&message, FILE_HEADER, fileName, strlen(fileName) + 1, seq);
	seq = sendUntilACK(&message, seq);

	return seq;
}

// Sends a file
int sendFile(int seq, int file) {
	msg message;
	int maxLen = MAXL - sizeof(KermitHeader) - sizeof(KermitTrailer);
	char buff[maxLen];

	int bytes;
	while ((bytes = read(file, &buff, maxLen)) > 0) {
		packMessage(&message, DATE, buff, bytes, seq);
		KermitHeader head;
		unpackHeader(&message, &head);
		seq = sendUntilACK(&message, seq);
	}

	return seq;
}

int sendEOF(int seq) {
	msg message;

	packMessage(&message, EoF, NULL, 0, seq);
	seq = sendUntilACK(&message, seq);

	return seq;
}

int sendEOT(int seq) {
	msg message;

	packMessage(&message, EOT, NULL, 0, seq);
	seq = sendUntilACK(&message, seq);

	return seq;
}

void sendFiles(char** fileNames, int count) {
	int seq = 0;

	seq = sendInit(seq);

	// Iterate file names
	for (int i = 0; i < count; i++) {
		int file = open(fileNames[i], O_RDONLY);
		if (file == -1) {
			printf("File %s does not exist!\n", fileNames[i]);
			fflush(stdout);
			exit(-1);
		}

		printf("Ksender: Sending file %s...\n", fileNames[i]);
		fflush(stdout);

		seq = sendFileHeader(seq, fileNames[i]);

		seq = sendFile(seq, file);

		close(file);
		seq = sendEOF(seq);
	}

	sendEOT(seq);
}

int main(int argc, char** argv) {
    init(HOST, PORT);

    sendFiles(argv + 1, argc - 1);
    
    return 0;
}