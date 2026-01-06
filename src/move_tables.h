#ifndef MOVE_TABLES_H
#define MOVE_TABLES_H

#include "definitions.h"

// extern U64 pawn_attacks[2][64];
// extern U64 knight_attacks[64];
// extern U64 king_attacks[64];
// extern U64 bishop_masks[64];
// extern U64 rook_masks[64];
// extern U64 bishop_attacks[64][512];
// extern U64 rook_attacks[64][4096];

extern U64 bishop_magic_numbers[64];
extern U64 rook_magic_numbers[64];

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



#endif