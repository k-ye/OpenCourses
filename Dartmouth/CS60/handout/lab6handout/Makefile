all: overlay/overlay network/network 
common/pkt.o: common/pkt.c common/pkt.h common/constants.h
	gcc -Wall -pedantic -std=c99 -g -c common/pkt.c -o common/pkt.o
topology/topology.o: topology/topology.c 
	gcc -Wall -pedantic -std=c99 -g -c topology/topology.c -o topology/topology.o
overlay/neighbortable.o: overlay/neighbortable.c
	gcc -Wall -pedantic -std=c99 -g -c overlay/neighbortable.c -o overlay/neighbortable.o
overlay/overlay: topology/topology.o common/pkt.o overlay/neighbortable.o overlay/overlay.c 
	gcc -Wall -pedantic -std=c99 -g -pthread overlay/overlay.c topology/topology.o common/pkt.o overlay/neighbortable.o -o overlay/overlay
network/network: common/pkt.o topology/topology.o network/network.c 
	gcc -Wall -pedantic -std=c99 -g -pthread common/pkt.o topology/topology.o network/network.c -o network/network 

clean:
	rm -rf common/*.o
	rm -rf topology/*.o
	rm -rf overlay/*.o
	rm -rf overlay/overlay
	rm -rf network/*.o
	rm -rf network/network 


