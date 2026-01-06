# main:
# # 	g++ -Ofast bitboard.cpp -o main
# 	g++ -std=c++17 bitboard.cpp -o main -g

all: chess_engine

chess_engine: main_debug.o board_debug.o move_tables.o definitions.o
	g++ -g main_debug.o board_debug.o move_tables.o definitions.o -o chess_engine

main_debug.o: main_debug.cpp
	g++ -c -g main_debug.cpp -std=c++17

board_debug.o: board_debug.cpp board_debug.h
	g++ -c -g board_debug.cpp -std=c++17

move_tables.o: move_tables.cpp move_tables.h
	g++ -c -g move_tables.cpp -std=c++17

definitions.o: definitions.cpp definitions.h
	g++ -c -g definitions.cpp -std=c++17

clean:
	rm -f *.o chess_engine