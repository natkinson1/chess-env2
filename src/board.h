#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <unordered_map>
#include <string.h>
#include <tuple>
#include <cstdio>
#include <cmath>
#include "move_tables.h"

#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "

#define get_move_source(move) (move & 0x3f)
#define get_move_target(move) ((move & 0xfc0) >> 6)
#define get_move_piece(move) ((move & 0xf000) >> 12)
#define get_move_promoted(move) ((move & 0xf0000) >> 16)
#define get_move_capture(move) (move & 0x100000)
#define get_move_double(move) (move & 0x200000)
#define get_move_enpassant(move) (move & 0x400000)
#define get_move_castling(move) (move & 0x800000)

#define copy_board()\
    U64 bitboards_copy[12], occupancies_copy[3]; \
    int side_copy, enpassant_copy, castle_copy; \
    memcpy(bitboards_copy, bitboards, 96); \
    memcpy(occupancies_copy, occupancies, 24); \
    side_copy = side, enpassant_copy = enpassant, castle_copy = castle; \

#define take_back() \
    memcpy(bitboards, bitboards_copy, 96); \
    memcpy(occupancies, occupancies_copy, 24); \
    side = side_copy, enpassant = enpassant_copy, castle = castle_copy; \

typedef struct {
    int moves[256];
    int count;
} moves;

extern U64 bitboards[12];
extern U64 occupancies[3];
extern int side;
extern int enpassant;
extern int castle;
extern int no_progress_count;
extern int current_state_pos;
extern int total_move_count;
extern std::unordered_map<U64, int> repetition_count;

void print_board();
void parse_fen(const char *fen);
static inline int is_square_attacked(int square, int side);
static inline void add_move(moves  *move_list, int move);
static inline int make_move(int move, int move_flag);
static inline void generate_moves(moves *move_list);
U64 hash_game_state();
void update_repition_count(U64 board_hash);
int get_repitition_count(U64 board_hash);
std::tuple<int, int> get_move_direction_and_distance(int source_square, int target_square);
int get_knight_move_direction(int source_square, int target_square);
std::vector<std::vector<int>> encode_board();
void update_state();
std::vector<std::vector<int>> get_ordered_state();
std::vector<std::vector<int>> reset();

#endif