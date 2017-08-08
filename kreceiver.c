#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

void checkError(msg* result) {
	if (result == NULL) {
		printf("Kreceiver: Link error!\n");
		fflush(stdout);
		exit(-1);
	}
}

// Sends ACK or NAK (from message) and waits for next packet with seq
msg* receivePacket(msg* message, int seq) {
	KermitHeader header;
	msg* receive;

	int retries = 0;
	int received = 0;
	while (retries < MAX_RETRIES && !received) {

		// Send ACK or NAK
		send_message(message);

		// Wait for next packet
		receive = receive_message_timeout(MAX_TIMEOUT);
		if (receive != NULL) {

			// Check if right seq number
			unpackHeader(receive, &header);
			if (header.seq == seq) {
				received = 1;
			}
			else {
				// Discard packet
				retries--;
			}
		}
		retries++;
	}

	if (received) {
		return receive;
	}

	// Timeout for 3 retries -> Link Error
	return NULL;
}

// Sends NAK until recieves a message with good CRC
int receiveUntilCRC(msg* message, msg** receive, int seq) {
	msg* rec;

	seq = incSeq(seq);
	rec = receivePacket(message, seq);
	checkError(rec);

	while (!checkCRC(rec)) {
		seq = incSeq(seq);
		packMessage(message, NAK, NULL, 0, seq);
		seq = incSeq(seq);
		rec = receivePacket(message, seq);
		checkError(rec);
	}

	(*receive) = rec;
	return seq;
}

void receiveFiles() {
	msg *rec;
	msg send;
	int seq = 0;
	char buffer[MAXL];

	// Receiving Send-Init
	msg* init = receive_message_timeout(3 * MAX_TIMEOUT);
	checkError(init);


	while (!checkCRC(init)) {
		seq = incSeq(seq);
		packMessage(&send, NAK, NULL, 0, seq);
		seq = incSeq(seq);
		init = receivePacket(&send, seq);
		checkError(init);
	}

	// Get info from Send-Init
	unpackData(init, buffer);

	seq = incSeq(seq);

	// Sending ACK with Send-Init data
	packMessage(&send, ACK, buffer, sizeof(InitData), seq);

	int loop = 1;
	while (loop) {
		// Receive next packet
		seq = receiveUntilCRC(&send, &rec, seq);

		KermitHeader header;
		unpackHeader(rec, &header);

		// If packet is EOT, exit
		if (header.type == EOT) {
			seq = incSeq(seq);
            packMessage(&send, ACK, NULL, 0, seq);
            send_message(&send);
            loop = 0;
            continue;
		}

		// Get file name from FILE_HEADER packet
		unpackData(rec, buffer);
		char fileName[MAXL] = "recv_";
		strcat(fileName, buffer);

		int file = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0644);

		printf("Kreceiver: Receiving file %s...\n", fileName);
		fflush(stdout);

		// Receiving file data
		int recv = 1;
		while (recv) {
			seq = incSeq(seq);
			packMessage(&send, ACK, 0, 0, seq);
			seq = receiveUntilCRC(&send, &rec, seq);

			unpackHeader(rec, &header);

			// If packet is EOF, stop receiving file data
			if (header.type == EoF) {
				recv = 0;
				continue;
			}

			// Write data to file
			int count = unpackData(rec, buffer);
			write(file, buffer, count);
		}
		close(file);

		printf("Kreceiver: File received!\n");
		fflush(stdout);

		seq = incSeq(seq);
		packMessage(&send, ACK, 0, 0, seq);
	}
}

int main(int argc, char** argv) {
    init(HOST, PORT);

	receiveFiles();

	return 0;
}