#include <cstdio>

#include "definitions.h"

int char_to_piece(char c) {
    switch(c) {
        case 'P': return P;
        case 'N': return N;
        case 'B': return B;
        case 'R': return R;
        case 'Q': return Q;
        case 'K': return K;
        case 'p': return p;
        case 'n': return n;
        case 'b': return b;
        case 'r': return r;
        case 'q': return q;
        case 'k': return k;
        default: return -1;
    }
};

char piece_to_promoted_char(int piece) {
    switch(piece) {
        case Q: return 'q';
        case R: return 'r';
        case B: return 'b';
        case N: return 'n';
        case q: return 'q';
        case r: return 'r';
        case b: return 'b';
        case n: return 'n';
    }
    return '\0';
}

void print_bitboard(U64 bitboard) {
    printf("\n");
    for (int rank = 0; rank < 8; rank++) {
        printf(" %d| ", 8 - rank);
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            printf(" %d ", get_bit(bitboard, square) ? 1 : 0);
        }
        printf("\n");
    }
    printf("   ------------------------");
    printf("\n");
    printf("     a  b  c  d  e  f  g  h");
    printf("\n");
    printf(" %llu\n\n ", bitboard);
}