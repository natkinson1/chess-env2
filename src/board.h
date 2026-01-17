#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <array>
#include <unordered_map>
#include <string.h>
#include <tuple>
#include <cstdio>
#include <cmath>

#include "definitions.h"

#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "


#define get_move_source(move) (move & 0x3f)
#define get_move_target(move) ((move & 0xfc0) >> 6)
#define get_move_piece(move) ((move & 0xf000) >> 12)
#define get_move_promoted(move) ((move & 0xf0000) >> 16)
#define get_move_capture(move) (move & 0x100000)
#define get_move_double(move) (move & 0x200000)
#define get_move_enpassant(move) (move & 0x400000)
#define get_move_castling(move) (move & 0x800000)

typedef struct {
    int moves[256];
    int count;
} moves;

struct State {
    U64 bitboards[12];
    U64 occupancies[3];
    int side;
    int enpassant;
    int castle;
};

static U64 pawn_attacks[2][64];
static U64 knight_attacks[64];
static U64 king_attacks[64];
static U64 bishop_masks[64];
static U64 rook_masks[64];
static U64 bishop_attacks[64][512];
static U64 rook_attacks[64][4096];
U64 get_bishop_attacks(int square, U64 occupancy);
U64 get_rook_attacks(int square, U64 occupancy);
U64 get_queen_attacks(int square, U64 occupancy);
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask);
U64 mask_pawn_attacks(int square, int side);
U64 mask_knight_attacks(int square);
U64 mask_king_attacks(int square);
U64 mask_bishop_attacks(int square);
U64 mask_rook_attacks(int square);
U64 bishop_attacks_on_fly(int square, U64 block);
U64 rook_attacks_on_fly(int square, U64 block);
void init_leaper_attacks();
unsigned int get_random_U32_number();
U64 get_random_U64_number();
U64 generate_magic_number();
U64 find_magic_number(int square, int relevant_bits, int bishop);
void init_magic_numbers();
void init_sliders_attacks(int bishop);
void init_all();

static inline void add_move(moves  *move_list, int move);

class Board {
private:
    int total_move_count = 0;
    int current_state_pos = 0;
    int no_progress_count = 0;
    int has_restarted = 0;
    int castle;
    int enpassant;
    int side;
    U64 bitboards[12];
    U64 occupancies[3];
    int castle_copy;
    int enpassant_copy;
    int side_copy;
    U64 bitboards_copy[12];
    U64 occupancies_copy[3];
    std::unordered_map<U64, int> repetition_count;
    std::unordered_map<int, int> move_index;
    int n_repititions;
    int prev_n_repititions = 0;
    std::vector<std::vector<int>> state;
    std::vector<std::vector<std::vector<int>>> state_history;
    void init_leaper_attacks();
    void init_sliders_attacks(int bishop);
    inline int is_square_attacked(int square, int side);
    inline int make_move(int move, int move_flag);
    inline void generate_moves(moves *move_list);
    U64 hash_game_state();
    void update_repition_count(U64 board_hash);
    int get_repitition_count(U64 board_hash);
    std::tuple<int, int> get_move_direction_and_distance(int source_square, int target_square);
    int get_knight_move_direction(int source_square, int target_square);
    void encode_board();
    void update_state();
    std::vector<std::vector<int>> get_ordered_state();
    inline void copy_board() {
        memcpy(this->bitboards_copy, this->bitboards, 96);
        memcpy(this->occupancies_copy, this->occupancies, 24);
        this->side_copy = this->side;
        this->enpassant_copy = this->enpassant;
        this->castle_copy = this->castle;
    }
    
    inline void take_back() {
        memcpy(this->bitboards, this->bitboards_copy, 96);
        memcpy(this->occupancies, this->occupancies_copy, 24);
        this->side = this->side_copy;
        this->enpassant = this->enpassant_copy;
        this->castle = this->castle_copy;
    }

public:
    Board() {
        init_all();
        memset(this->occupancies, 0ULL, 24);
        memset(this->bitboards, 0ULL, 96);
        this->state_history.reserve(16);
    }
    void print_board();
    void parse_fen(const char *fen);
    std::tuple<std::vector<std::vector<int>>, int, int> reset();
    std::vector<std::vector<int>> get_legal_moves();
    std::tuple<std::vector<std::vector<int>>, int, int> step(int action_idx);
    State save_state() {
        State state;
        memcpy(state.bitboards, this->bitboards, 96);
        memcpy(state.occupancies, this->occupancies, 24);
        state.side = this->side;
        state.enpassant = this->enpassant;
        state.castle = this->castle;

        return state;
    }
    
    inline void restore_state(State state) {
        memcpy(this->bitboards, state.bitboards, 96);
        memcpy(this->occupancies, state.occupancies, 24);
        this->side = state.side;
        this->enpassant = state.enpassant;
        this->castle = state.castle;
    }
};


#endif