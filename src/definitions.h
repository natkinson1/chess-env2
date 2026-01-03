#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define U64 unsigned long long
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define pop_bit(bitboard, square) (get_bit(bitboard, square) ? bitboard ^= (1ULL << square) : 0)
#define encode_move(source, target, piece, promoted, capture, double, enpassant, castling) \
(source) | (target << 6) | (piece << 12) | (promoted << 16) | (capture << 20) | (double << 21) | (enpassant << 22) | (castling << 23)

enum { white, black, both };
enum { rook, bishop };
enum { all_moves, only_captures };
enum { north, north_east, east, south_east, south, south_west, west, north_west};
enum {knight_nw, knight_ne, knight_en, knight_es, knight_se, knight_sw, knight_ws, knight_wn };

int char_to_piece(char c);
char piece_to_promoted_char(int piece);
void print_bitboard(U64 bitboard);

static inline int count_bits(U64 bitboard) {
    int count = 0;

    while (bitboard) {
        count ++;
        bitboard &= bitboard - 1;
        
    }
    return count;
}

static inline int get_ls1b_index(U64 bitboard) {
    if (bitboard) {
        // return ctzll(bitboard);
        // return __builtin_ctzll(bitboard);
        return count_bits((bitboard & -bitboard) - 1);
    } else {
        return -1;
    }
}

enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};



enum {wk = 1, wq = 2, bk = 4, bq = 8};

// encode pieces: white, black
enum { P, N, B, R, Q, K, p, n, b, r, q, k};

const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;
const U64 not_ab_file = 18229723555195321596ULL;
const U64 not_gh_file = 4557430888798830399ULL;

static const char *square_to_coord[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

const int castling_rights[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

const int bishop_relevant_bits[64] = {
    6,  5,  5,  5,  5,  5,  5,  6,
    5,  5,  5,  5,  5,  5,  5,  5,
    5,  5,  7,  7,  7,  7,  5,  5,
    5,  5,  7,  9,  9,  7,  5,  5,
    5,  5,  7,  9,  9,  7,  5,  5,
    5,  5,  7,  7,  7,  7,  5,  5,
    5,  5,  5,  5,  5,  5,  5,  5,
    6,  5,  5,  5,  5,  5,  5,  6
};

const int rook_relevant_bits[64] = {
    12,  11,  11,  11,  11,  11,  11,  12,
    11,  10,  10,  10,  10,  10,  10,  11,
    11,  10,  10,  10,  10,  10,  10,  11,
    11,  10,  10,  10,  10,  10,  10,  11,
    11,  10,  10,  10,  10,  10,  10,  11,
    11,  10,  10,  10,  10,  10,  10,  11,
    11,  10,  10,  10,  10,  10,  10,  11,
    12,  11,  11,  11,  11,  11,  11,  12
};

static const char *unicode_pieces[12] = {
    "♙", "♘", "♗", "♖", "♕", "♔", "♟", "♞", "♝", "♜", "♛", "♚"
};

static const char ascii_pieces[] = "PNBRQKpnbrqk";

#endif