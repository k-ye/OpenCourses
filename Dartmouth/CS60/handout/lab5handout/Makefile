all: simple stress

simple: client/app_simple_client.o server/app_simple_server.o client/srt_client.o server/srt_server.o common/seg.o
	gcc -g -pthread server/app_simple_server.o common/seg.o server/srt_server.o -o server/simple_server
	gcc -g -pthread client/app_simple_client.o common/seg.o client/srt_client.o -o client/simple_client

stress: client/app_stress_client.o server/app_stress_server.o client/srt_client.o server/srt_server.o common/seg.o
	gcc -g -pthread server/app_stress_server.o common/seg.o server/srt_server.o -o server/stress_server
	gcc -g -pthread client/app_stress_client.o common/seg.o client/srt_client.o -o client/stress_client

client/app_simple_client.o: client/app_simple_client.c 
	gcc -pthread -g -c client/app_simple_client.c -o client/app_simple_client.o 
server/app_simple_server.o: server/app_simple_server.c 
	gcc -pthread -g -c server/app_simple_server.c -o server/app_simple_server.o

client/app_stress_client.o: client/app_stress_client.c 
	gcc -pthread -g -c client/app_stress_client.c -o client/app_stress_client.o 
server/app_stress_server.o: server/app_stress_server.c 
	gcc -pthread -g -c server/app_stress_server.c -o server/app_stress_server.o

common/seg.o: common/seg.c common/seg.h
	gcc -g -c common/seg.c -o common/seg.o
client/srt_client.o: client/srt_client.c client/srt_client.h 
	gcc -pthread -g -c client/srt_client.c -o client/srt_client.o
server/srt_server.o: server/srt_server.c server/srt_server.h
	gcc -pthread -g -c server/srt_server.c -o server/srt_server.o

clean:
	rm -rf client/*.o
	rm -rf server/*.o
	rm -rf common/*.o
	rm -rf client/simple_client
	rm -rf server/simple_server
	rm -rf client/stress_client
	rm -rf server/stress_server

