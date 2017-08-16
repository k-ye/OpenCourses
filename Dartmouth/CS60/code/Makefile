all: simple stress lab4

CC=gcc
CFLAGS= -g -pthread -Wall -Wno-unused-command-line-argument

simple: client/app_simple_client.o server/app_simple_server.o client/srt_client.o server/srt_server.o common/o
	$(CC) $(CFLAGS) common/*.o server/app_simple_server.o server/srt_server.o -o server/simple_server
	$(CC) $(CFLAGS) common/*.o client/app_simple_client.o client/srt_client.o -o client/simple_client

stress: client/app_stress_client.o server/app_stress_server.o client/srt_client.o server/srt_server.o common/o
	$(CC) $(CFLAGS) common/*.o server/app_stress_server.o server/srt_server.o -o server/stress_server
	$(CC) $(CFLAGS) common/*.o client/app_stress_client.o client/srt_client.o -o client/stress_client

lab4: server/app_server.o client/app_client.o server/srt_server.o client/srt_client.o common/o
	$(CC) $(CFLAGS) common/*.o server/app_server.o server/srt_server.o -o server/app_server
	$(CC) $(CFLAGS) common/*.o client/app_client.o client/srt_client.o -o client/app_client

client/app_simple_client.o: client/app_simple_client.c 
	$(CC) $(CFLAGS) -c client/app_simple_client.c -o client/app_simple_client.o

server/app_simple_server.o: server/app_simple_server.c 
	$(CC) $(CFLAGS) -c server/app_simple_server.c -o server/app_simple_server.o

client/app_stress_client.o: client/app_stress_client.c 
	$(CC) $(CFLAGS) -c client/app_stress_client.c -o client/app_stress_client.o 

server/app_stress_server.o: server/app_stress_server.c 
	$(CC) $(CFLAGS) -c server/app_stress_server.c -o server/app_stress_server.o

client/app_client.o: client/app_client.c 
	$(CC) $(CFLAGS) -c client/app_client.c -o client/app_client.o 

server/app_server.o: server/app_server.c 
	$(CC) $(CFLAGS) -c server/app_server.c -o server/app_server.o

client/srt_client.o: client/srt_client.c client/srt_client.h 
	$(CC) $(CFLAGS) -c client/srt_client.c -o client/srt_client.o

server/srt_server.o: server/srt_server.c server/srt_server.h
	$(CC) $(CFLAGS) -c server/srt_server.c -o server/srt_server.o

common/o: common/seg.o common/event.o common/util.o

common/seg.o: common/seg.c common/seg.h
	$(CC) -g -c common/seg.c -o common/seg.o

common/event.o: common/event.h common/event.c
	$(CC) -g -c common/event.c -o common/event.o

common/util.o: common/util.h common/util.c
	$(CC) -g -c common/util.c -o common/util.o

clean:
	rm -rf client/*.o
	rm -rf server/*.o
	rm -rf common/*.o
	rm -rf client/simple_client
	rm -rf server/simple_server
	rm -rf client/stress_client
	rm -rf server/stress_server
	rm -rf client/app_client
	rm -rf server/app_server