all: chess_engine

chess_engine: debug.o board.o definitions.o
	g++ -g debug.o board.o definitions.o -o chess_engine

debug.o: debug.cpp
	g++ -c -g debug.cpp -std=c++17

board.o: board.cpp board.h
	g++ -c -g board.cpp -std=c++17

definitions.o: definitions.cpp definitions.h
	g++ -c -g definitions.cpp -std=c++17

clean:
	rm -f *.o chess_engine