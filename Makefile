HEADERS=patch.h file.h
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

canonicalizeplanargraph: canonicalizeplanargraph.cc patch.cc growthpairs.cc file.cc
	g++ $(GXXOPTS) -o canonicalizeplanargraph patch.cc canonicalizeplanargraph.cc growthpairs.cc file.cc

canonicalizeplanargraph-nole: canonicalizeplanargraph.cc patch.cc growthpairs.cc file.cc
	g++ $(GXXOPTS) -DPLANARCODE_NO_LE -o canonicalizeplanargraph-nole patch.cc canonicalizeplanargraph.cc growthpairs.cc file.cc

clean:
	rm $(OBJECTS) main3.o main3 canonicalizeplanargraph canonicalizeplanargraph-nole
