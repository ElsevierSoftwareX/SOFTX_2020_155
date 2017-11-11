all: fe_stream_test

fe_stream_test.o: fe_stream_test.cc gps.hh ../drv/rfm.c ../drv/mbuf/mbuf.c str_split.hh
	g++ -c -O -g3 fe_stream_test.cc -o fe_stream_test.o -I.

str_split.o: str_split.cc str_split.hh
	g++ -c -O -g3 str_split.cc -o str_split.o -I.

fe_stream_test: fe_stream_test.o str_split.o
	g++ fe_stream_test.o str_split.o -o fe_stream_test -lrt

clean:
	rm fe_stream_test.o fe_stream_test
