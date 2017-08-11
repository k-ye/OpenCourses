all: overlay/overlay network/network client/app_simple_client server/app_simple_server client/app_stress_client server/app_stress_server   

common/pkt.o: common/pkt.c common/pkt.h common/constants.h
	gcc -Wall -pedantic -std=c99 -g -c common/pkt.c -o common/pkt.o
topology/topology.o: topology/topology.c 
	gcc -Wall -pedantic -std=c99 -g -c topology/topology.c -o topology/topology.o
overlay/neighbortable.o: overlay/neighbortable.c
	gcc -Wall -pedantic -std=c99 -g -c overlay/neighbortable.c -o overlay/neighbortable.o
overlay/overlay: topology/topology.o common/pkt.o overlay/neighbortable.o overlay/overlay.c 
	gcc -Wall -pedantic -std=c99 -g -pthread overlay/overlay.c topology/topology.o common/pkt.o overlay/neighbortable.o -o overlay/overlay
network/nbrcosttable.o: network/nbrcosttable.c
	gcc -Wall -pedantic -std=c99 -g -c network/nbrcosttable.c -o network/nbrcosttable.o
network/dvtable.o: network/dvtable.c
	gcc -Wall -pedantic -std=c99 -g -c network/dvtable.c -o network/dvtable.o
network/routingtable.o: network/routingtable.c
	gcc -Wall -pedantic -std=c99 -g -c network/routingtable.c -o network/routingtable.o
network/network: common/pkt.o common/seg.o topology/topology.o network/nbrcosttable.o network/dvtable.o network/routingtable.o network/network.c 
	gcc -Wall -pedantic -std=c99 -g -pthread network/nbrcosttable.o  network/dvtable.o network/routingtable.o common/pkt.o common/seg.o topology/topology.o network/network.c -o network/network 
client/app_simple_client: client/app_simple_client.c common/seg.o client/srt_client.o topology/topology.o 
	gcc -Wall -pedantic -std=c99 -g -pthread client/app_simple_client.c common/seg.o client/srt_client.o topology/topology.o -o client/app_simple_client 
client/app_stress_client: client/app_stress_client.c common/seg.o client/srt_client.o topology/topology.o 
	gcc -Wall -pedantic -std=c99 -g -pthread client/app_stress_client.c common/seg.o client/srt_client.o topology/topology.o -o client/app_stress_client 
server/app_simple_server: server/app_simple_server.c common/seg.o server/srt_server.o topology/topology.o 
	gcc -Wall -pedantic -std=c99 -g -pthread server/app_simple_server.c common/seg.o server/srt_server.o topology/topology.o -o server/app_simple_server
server/app_stress_server: server/app_stress_server.c common/seg.o server/srt_server.o topology/topology.o 
	gcc -Wall -pedantic -std=c99 -g -pthread server/app_stress_server.c common/seg.o server/srt_server.o topology/topology.o -o server/app_stress_server
common/seg.o: common/seg.c common/seg.h
	gcc -Wall -pedantic -std=c99 -g -c common/seg.c -o common/seg.o
client/srt_client.o: client/srt_client.c client/srt_client.h 
	gcc -Wall -pedantic -std=c99 -g -c client/srt_client.c -o client/srt_client.o
server/srt_server.o: server/srt_server.c server/srt_server.h
	gcc -Wall -pedantic -std=c99 -g -c server/srt_server.c -o server/srt_server.o

clean:
	rm -rf common/*.o
	rm -rf topology/*.o
	rm -rf overlay/*.o
	rm -rf overlay/overlay
	rm -rf network/*.o
	rm -rf network/network 
	rm -rf client/*.o
	rm -rf server/*.o
	rm -rf client/app_simple_client
	rm -rf client/app_stress_client
	rm -rf server/app_simple_server
	rm -rf server/app_stress_server
	rm -rf server/receivedtext.txt



