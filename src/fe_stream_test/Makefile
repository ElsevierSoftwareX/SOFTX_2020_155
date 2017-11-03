all: fe_stream_test

fe_stream_test.o: fe_stream_test.cc gps.hh ../drv/rfm.c ../drv/mbuf/mbuf.c
	g++ -c -O -g3 fe_stream_test.cc -o fe_stream_test.o -I.

fe_stream_test: fe_stream_test.o
	g++ fe_stream_test.o -o fe_stream_test -lrt

clean:
	rm fe_stream_test.o fe_stream_test
