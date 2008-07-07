HEADERS=patch.h file.h canonical_border_int.h
FILES=main3.cc patch.cc file.cc
OBJECTS=patch.o growthpairs.o file.o

GXXOPTS=-O3 -g -Wall

all: main3 canonicalizeplanargraph canonicalizeplanargraph-nole

main3: $(OBJECTS) main3.o growthpairs.o
	g++ $(GXXOPTS) -o main3 $(OBJECTS) main3.o

main3.o: main3.cc $(HEADERS) growthpairs.h
	g++ $(GXXOPTS) -c -o main3.o main3.cc

growthpairs.o: growthpairs.cc $(HEADERS) growthpairs.h
	g++ $(GXXOPTS) -c -o growthpairs.o growthpairs.cc

file.o: file.cc $(HEADERS) file.h
	g++ $(GXXOPTS) -c -o file.o file.cc

patch.o: patch.cc $(HEADERS)
	g++ $(GXXOPTS) -c -o patch.o patch.cc

canonicalizeplanargraph: canonicalizeplanargraph.cc patch.cc growthpairs.cc file.cc $(HEADERS)
	g++ $(GXXOPTS) -o canonicalizeplanargraph patch.cc canonicalizeplanargraph.cc growthpairs.cc file.cc

canonicalizeplanargraph-nole: canonicalizeplanargraph.cc patch.cc growthpairs.cc file.cc $(HEADERS)
	g++ $(GXXOPTS) -DPLANARCODE_NO_LE -o canonicalizeplanargraph-nole patch.cc canonicalizeplanargraph.cc growthpairs.cc file.cc

canonical_border_test: canonical_border_test.cc canonical_border_int.h
	g++ $(GXXOPTS) -O0 -o canonical_border_test canonical_border_test.cc

clean:
	rm $(OBJECTS) main3.o main3 canonicalizeplanargraph canonicalizeplanargraph-nole canonical_border_test
