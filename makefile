SRC = src/
BIN = bin/
BENCH = bench/

CC = cc
CXX = c++
CSWITCHES = -O -DNO_TIMER -DANSI_DECLARATORS -I$(SRC)
TRILIBDEFS = -DTRILIBRARY
RM = /bin/rm -f

all: $(BIN)triangle $(BIN)benchmark_hull $(BIN)benchmark_delaunay $(BIN)benchmark_voronoi $(BIN)Triangle.o $(BIN)test_modern_api

trilibrary: $(BIN)triangle.o $(BIN)tricall

$(BIN)triangle.o: $(SRC)triangle.c $(SRC)triangle.h
	$(CC) $(CSWITCHES) $(TRILIBDEFS) -c -o $(BIN)triangle.o $(SRC)triangle.c

$(BIN)Triangle.o: $(SRC)Triangle.cpp $(SRC)Triangle.hpp
	$(CXX) $(CSWITCHES) -std=c++11 -c -o $(BIN)Triangle.o $(SRC)Triangle.cpp

$(BIN)test_modern_api: test_modern_api.cpp $(BIN)Triangle.o $(BIN)triangle.o
	$(CXX) $(CSWITCHES) -std=c++11 -o $(BIN)test_modern_api test_modern_api.cpp $(BIN)Triangle.o $(BIN)triangle.o -lm

$(BIN)benchmark_hull: $(BENCH)benchmark_hull.c $(BIN)triangle.o
	$(CC) $(CSWITCHES) -o $(BIN)benchmark_hull $(BENCH)benchmark_hull.c $(BIN)triangle.o -lm

$(BIN)benchmark_delaunay: $(BENCH)benchmark_delaunay.c $(BIN)triangle.o
	$(CC) $(CSWITCHES) -o $(BIN)benchmark_delaunay $(BENCH)benchmark_delaunay.c $(BIN)triangle.o -lm

$(BIN)benchmark_voronoi: $(BENCH)benchmark_voronoi.c $(BIN)triangle.o
	$(CC) $(CSWITCHES) -o $(BIN)benchmark_voronoi $(BENCH)benchmark_voronoi.c $(BIN)triangle.o -lm

$(BIN)triangle: $(SRC)triangle.c
	$(CC) $(CSWITCHES) -o $(BIN)triangle $(SRC)triangle.c -lm

$(BIN)tricall: $(SRC)tricall.c $(BIN)triangle.o
	$(CC) $(CSWITCHES) -o $(BIN)tricall $(SRC)tricall.c $(BIN)triangle.o -lm

$(BIN)showme: $(SRC)showme.c
	$(CC) $(CSWITCHES) -o $(BIN)showme $(SRC)showme.c -lX11

distclean:
	$(RM) $(BIN)triangle $(BIN)triangle.o $(BIN)tricall $(BIN)showme \
		$(BIN)benchmark_hull $(BIN)benchmark_delaunay $(BIN)benchmark_voronoi $(BIN)Triangle.o $(BIN)test_modern_api $(BIN)test_voroface
lean:
	$(RM) $(BIN)triangle $(BIN)triangle.o $(BIN)tricall $(BIN)showme \
		$(BIN)benchmark_hull $(BIN)benchmark_delaunay $(BIN)benchmark_voronoi $(BIN)Triangle.o $(BIN)test_modern_api
