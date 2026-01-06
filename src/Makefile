all: chess_engine

chess_engine: main_debug.o board.o definitions.o
	g++ -g main_debug.o board.o definitions.o -o chess_engine

main_debug.o: main_debug.cpp
	g++ -c -g main_debug.cpp -std=c++17

board.o: board.cpp board.h
	g++ -c -g board.cpp -std=c++17

definitions.o: definitions.cpp definitions.h
	g++ -c -g definitions.cpp -std=c++17

clean:
	rm -f *.o chess_engine