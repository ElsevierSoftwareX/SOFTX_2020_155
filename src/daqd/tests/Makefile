test_daqd_cmask_t: test_daqd_cmask_t.o test_main.o
	g++ test_daqd_cmask_t.o test_main.o -o test_daqd_cmask_t

test_daqd_cmask_t.o: test_daqd_cmask_t.cc ../cmask_t.h
	g++ -c test_daqd_cmask_t.cc -o test_daqd_cmask_t.o -I.. -std=c++0x

test_main.o: test_main.cc
	g++ -c test_main.cc -o test_main.o -std=c++0x -I..

test_nds1_connections: test_nds1_connections.o
	g++ test_nds1_connections.o -lndscxx -lpthread -o test_nds1_connections

test_nds1_connections.o: test_nds1_connections.cc
	g++ -c test_nds1_connections.cc -std=c++0x -I.. -I/usr/include/nds2-client


clean:
	rm *.o test_daqd_cmask_t test_nds1_connections
