# main:
# # 	g++ -Ofast bitboard.cpp -o main
# 	g++ -std=c++17 bitboard.cpp -o main -g

all: chess_engine

chess_engine: main.o board.o move_tables.o definitions.o
	g++ main.o board.o move_tables.o definitions.o -o chess_engine

main.o: main.cpp
	g++ -c main.cpp -std=c++17 -O3

board.o: board.cpp board.h
	g++ -c board.cpp -std=c++17 -O3

move_tables.o: move_tables.cpp move_tables.h
	g++ -c move_tables.cpp -std=c++17 -O3

definitions.o: definitions.cpp definitions.h
	g++ -c definitions.cpp -std=c++17 -O3

clean:
	rm -f *.o chess_engine