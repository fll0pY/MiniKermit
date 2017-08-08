all: build 

build: ksender kreceiver

ksender: ksender.o utils.o link_emulator/lib.o
	gcc -g ksender.o utils.o link_emulator/lib.o -o ksender

kreceiver: kreceiver.o utils.o link_emulator/lib.o
	gcc -g kreceiver.o utils.o link_emulator/lib.o -o kreceiver

.c.o: 
	gcc -Wall -g -c $? 

clean:
	-rm -f *.o ksender kreceiver 
	-rm -f ksender kreceiver