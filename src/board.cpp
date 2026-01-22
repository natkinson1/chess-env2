#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "board.h"

namespace py = pybind11;

void Board::print_board() {
    printf("\n");
    for (int rank = 0; rank < 8; rank++) {
        printf(" %d|", 8 - rank);
        py::print(" ", 8 - rank, "|", py::arg("end") = "");
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            int piece = -1;
            for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                if (get_bit(this->bitboards[bb_piece], square)) {
                    piece = bb_piece;
                }
            }
            printf("  %c", (piece == -1) ? '.' : ascii_pieces[piece]);
            if (piece == -1) {
                py::print("   .", py::arg("end") = "");
            } else {
                py::print(" ", unicode_pieces[piece], py::arg("end") = "");
            }
        }
        printf("\n");
        py::print("\n");
    }
    printf("   ------------------------");
    printf("\n");
    printf("     a  b  c  d  e  f  g  h");
    printf("\n\n");
    printf("side:       %s\n", !this->side ? "white" : "black");
    printf("enpassant:  %s\n", (this->enpassant != no_sq) ? square_to_coord[this->enpassant] : "NA");
    printf("Castling:   %c%c%c%c\n\n\n", (this->castle & wk) ? 'K' : '-', 
                                     (this->castle & wq) ? 'Q' : '-', 
                                     (this->castle & bk) ? 'k' : '-', 
                                     (this->castle & bq) ? 'q' : '-');
    py::print("      -------------------------------");
    py::print("       a   b   c   d   e   f   g   h");

}

void Board::parse_fen(const char *fen) {
    memset(this->bitboards, 0ULL, 96);
    memset(this->occupancies, 0ULL, 24);
    this->side = 0;
    this->enpassant = no_sq;
    this->castle = 0;
    this->no_progress_count = 0;

    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            if ((*fen >= 'a' && *fen <= 'z') | (*fen >= 'A' && *fen <= 'Z')) {
                int piece = char_to_piece(*fen);
                set_bit(this->bitboards[piece], square);

                fen++;
            }

            if (*fen >= '0' && *fen <= '9') {
                int offset = *fen - '0'; // convert char 0 -> int 0
                int piece = -1;
                // if piece on current square -> get piece code
                for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                    if (get_bit(this->bitboards[bb_piece], square)) {
                        piece = bb_piece;
                    }
                }
                if (piece == -1) {
                    file--;
                }
                file += offset;
                fen++;
            }

            if (*fen == '/') {
                fen++;
            }
        }
    }
    // skip blank space in fen
    fen++;
    (*fen == 'w') ? (this->side = white) : (this->side = black);
    
    // castling rights
    fen += 2;
    while (*fen != ' ') {
        switch (*fen) {
            case 'K': 
                this->castle |= wk;
                break;
            case 'k': 
                this->castle |= bk;
                break;
            case 'Q': 
                this->castle |= wq;
                break;
            case 'q': 
                this->castle |= bq;
                break;
            case '-':
                break;

        }
        fen++;
    }
    // parse enpassant square
    fen++;

    if (*fen != '-') {
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');
        this->enpassant = rank * 8 + file;
    } else {
        this->enpassant = no_sq;
    }

    // loop over white pieces
    for (int piece = P; piece <= K; piece++) {
        this->occupancies[white] |= this->bitboards[piece];
    }
    // black pieces
    for (int piece = p; piece <= k; piece++) {
        this->occupancies[black] |= this->bitboards[piece];
    }

    // both
    this->occupancies[both] |= this->occupancies[white];
    this->occupancies[both] |= this->occupancies[black];

};

void Board::init_leaper_attacks() {

    for (int square = 0; square < 64; square++) {
        pawn_attacks[white][square] = mask_pawn_attacks(square, white);
        pawn_attacks[black][square] = mask_pawn_attacks(square, black);

        knight_attacks[square] = mask_knight_attacks(square);

        king_attacks[square] = mask_king_attacks(square);
    }
};

void init_all() {
    init_leaper_attacks();
    init_sliders_attacks(bishop);
    init_sliders_attacks(rook);
    // init_magic_numbers();
}

inline int Board::is_square_attacked(int square, int side) {

    if ((side == white) && (pawn_attacks[black][square] & this->bitboards[P])) {
        return 1;
    }
    // black pawns
    if ((side == black) && (pawn_attacks[white][square] & this->bitboards[p])) {
        return 1;
    }
    // knight attacks
    if (knight_attacks[square] & ((side == white) ? this->bitboards[N] : this->bitboards[n])) {
        return 1;
    }
    if (get_bishop_attacks(square, this->occupancies[both]) & ((side == white) ? this->bitboards[B] : this->bitboards[b])) {
        return 1;
    }
    if (get_rook_attacks(square, this->occupancies[both]) & ((side == white) ? this->bitboards[R] : this->bitboards[r])) {
        return 1;
    }
    if (get_queen_attacks(square, this->occupancies[both]) & ((side == white) ? this->bitboards[Q] : this->bitboards[q])) {
        return 1;
    }
    // king attacks
    if (king_attacks[square] & ((side == white) ? this->bitboards[K] : this->bitboards[k])) {
        return 1;
    }

    return 0;
}

static inline void add_move(moves  *move_list, int move) {
    // store move
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

inline int Board::make_move(int move, int move_flag) {

    if (move_flag == all_moves) {
        this->copy_board();

        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted_piece = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castling(move);

        if (piece == P || piece == p) {
            this->no_progress_count = 0;
        }
        
        pop_bit(this->bitboards[piece], source_square);
        set_bit(this->bitboards[piece], target_square);

        if (capture) {
            this->no_progress_count = 0;
            int start_piece, end_piece;
            if (this->side == white) {
                start_piece = p;
                end_piece = k;
            } else {
                start_piece = P;
                end_piece = K;
            }
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) {
                if (get_bit(this->bitboards[bb_piece], target_square)) {
                    pop_bit(this->bitboards[bb_piece], target_square);
                    break;
                }
            }
        }
        // pawn promotions
        if (promoted_piece) {
            this->no_progress_count = 0;
            pop_bit(this->bitboards[(this->side == white) ? P : p], target_square);
            set_bit(this->bitboards[promoted_piece], target_square);
        }

        if (enpass) {
            this->no_progress_count = 0;
            (this->side == white) ? pop_bit(this->bitboards[p], target_square + 8) : pop_bit(this->bitboards[P], target_square - 8);
        }
        this->enpassant = no_sq;

        if (double_push) {
            this->no_progress_count = 0;
            (this->side == white) ? (this->enpassant = target_square + 8) : (this->enpassant = target_square - 8);
        }

        if (castling) {
            switch (target_square) {
                // move respective rook.
                case (g1):
                    pop_bit(this->bitboards[R], h1);
                    set_bit(this->bitboards[R], f1);
                    break;
                case (c1):
                    pop_bit(this->bitboards[R], a1);
                    set_bit(this->bitboards[R], d1);
                    break;
                case (g8):
                    pop_bit(this->bitboards[r], h8);
                    set_bit(this->bitboards[r], f8);
                    break;
                case (c8):
                    pop_bit(this->bitboards[r], a8);
                    set_bit(this->bitboards[r], d8);
                    break;
            }
        }
        // update castling rights
        this->castle &= castling_rights[source_square];
        this->castle &= castling_rights[target_square];

        // update occupancies
        memset(this->occupancies, 0ULL, 24);
        
        for (int bb_piece = P; bb_piece <= K; bb_piece++) {
            this->occupancies[white] |= this->bitboards[bb_piece];
        };
        for (int bb_piece = p; bb_piece <= k; bb_piece++) {
            this->occupancies[black] |= this->bitboards[bb_piece];
        }
        this->occupancies[both] |= this->occupancies[white];
        this->occupancies[both] |= this->occupancies[black];

        // change this->side
        this->side ^= 1;
        this->no_progress_count++;

        // ensure king isn't in check
        if (is_square_attacked((this->side == white ? get_ls1b_index(this->bitboards[k]) : get_ls1b_index(this->bitboards[K])), this->side)) {
            // move is illegal
            this->no_progress_count--;
            this->take_back();
            return 0;
        } else {
            return 1;
        }

    } else {
        // make sure move is the capture
        if (get_move_capture(move)) {
            make_move(move, all_moves);
        } else {
            return 0;
        }
    }
    return 0;
}

inline void Board::generate_moves(moves *move_list) {

    move_list->count = 0;

    int source_square, target_square;

    U64 bitboard, attacks;

    for (int piece = P; piece <= k; piece++) {
        bitboard = this->bitboards[piece];

        // generate white pawn and white king castling
        if (this->side == white) {
            if (piece == P) {
                while (bitboard) {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square - 8;

                    if (!(target_square < a8) && !get_bit(this->occupancies[both], target_square)) {
                        // pawn promotion
                        if ((source_square >= a7) && (source_square <= h7)) {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        } else {
                            // one square ahead
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead
                            if ((source_square >= a2 && source_square <= h2) && !get_bit(this->occupancies[both], target_square - 8)) {
                                add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                            }

                        }
                    }
                    attacks = pawn_attacks[this->side][source_square] & this->occupancies[black];

                    while (attacks) {
                        target_square = get_ls1b_index(attacks);

                        // pawn promotion
                        if ((source_square >= a7) && (source_square <= h7)) {
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        } else {
                            // one square ahead
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }

                        pop_bit(attacks, target_square);
                    }
                    if (this->enpassant != no_sq) {
                        U64 enpassant_attacks = pawn_attacks[this->side][source_square] & (1ULL << this->enpassant);
                        if (enpassant_attacks) {
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    pop_bit(bitboard, source_square);
                }
            }
            if (piece == K) {
                // king this->side castling available
                if (this->castle & wk) {
                    // ensure clear path
                    if (!get_bit(this->occupancies[both], f1) && !get_bit(this->occupancies[both], g1)) {
                        // doesn't move through check
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black)) {
                            add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }

                // queen this->side castling available
                if (this->castle & wq) {
                    if (!get_bit(this->occupancies[both], d1) && !get_bit(this->occupancies[both], c1) && !get_bit(this->occupancies[both], b1)) {
                        // doesn't move through check
                        if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black)) {
                            add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        } else {
            if (piece == p) {
                while (bitboard) {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square + 8;

                    if (!(target_square > h1) && !get_bit(this->occupancies[both], target_square)) {
                        // pawn promotion
                        if ((source_square >= a2) && (source_square <= h2)) {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        } else {
                            // one square ahead
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead
                            if ((source_square >= a7 && source_square <= h7) && !get_bit(this->occupancies[both], target_square + 8)) {
                                add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                            }

                        }
                        
                    }
                    attacks = pawn_attacks[this->side][source_square] & this->occupancies[white];

                    while (attacks) {
                        target_square = get_ls1b_index(attacks);

                        // pawn promotion
                        if ((source_square >= a2) && (source_square <= h2)) {
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        } else {
                            // one square ahead
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }

                        pop_bit(attacks, target_square);
                    }
                    if (this->enpassant != no_sq) {
                        U64 enpassant_attacks = pawn_attacks[this->side][source_square] & (1ULL << this->enpassant);
                        if (enpassant_attacks) {
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    pop_bit(bitboard, source_square);
                }
            }
            if (piece == k) {
                // king this->side castling available
                if (this->castle & bk) {
                    // ensure clear path
                    if (!get_bit(this->occupancies[both], f8) && !get_bit(this->occupancies[both], g8)) {
                        // doesn't move through check
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white)) {
                            add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }

                // queen this->side castling available
                if (this->castle & bq) {
                    if (!get_bit(this->occupancies[both], d8) && !get_bit(this->occupancies[both], c8) && !get_bit(this->occupancies[both], b8)) {
                        // doesn't move through check
                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white)) {
                            add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }

        // generate knight moves
        if ((this->side == white) ? piece == N : piece == n) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = knight_attacks[source_square] & ((this->side == white) ? ~this->occupancies[white] : ~this->occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((this->side == white) ? this->occupancies[black] : this->occupancies[white], target_square)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    } else {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    }
                    
                    pop_bit(attacks, target_square);
                }


                pop_bit(bitboard, source_square);
            }
        }

        // generate bishop moves
        if ((this->side == white) ? piece == B : piece == b) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = get_bishop_attacks(source_square, this->occupancies[both]) & ((this->side == white) ? ~this->occupancies[white] : ~this->occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((this->side == white) ? this->occupancies[black] : this->occupancies[white], target_square)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    } else {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    }
                    
                    pop_bit(attacks, target_square);
                }


                pop_bit(bitboard, source_square);
            }
        }

        // generate rook moves
        if ((this->side == white) ? piece == R : piece == r) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = get_rook_attacks(source_square, this->occupancies[both]) & ((this->side == white) ? ~this->occupancies[white] : ~this->occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((this->side == white) ? this->occupancies[black] : this->occupancies[white], target_square)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    } else {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    }
                    
                    pop_bit(attacks, target_square);
                }


                pop_bit(bitboard, source_square);
            }
        }

        // generate queen moves
        if ((this->side == white) ? piece == Q : piece == q) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = get_queen_attacks(source_square, this->occupancies[both]) & ((this->side == white) ? ~this->occupancies[white] : ~this->occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((this->side == white) ? this->occupancies[black] : this->occupancies[white], target_square)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    } else {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    }
                    
                    pop_bit(attacks, target_square);
                }


                pop_bit(bitboard, source_square);
            }
        }

        // generate king moves
        if ((this->side == white) ? piece == K : piece == k) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = king_attacks[source_square] & ((this->side == white) ? ~this->occupancies[white] : ~this->occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((this->side == white) ? this->occupancies[black] : this->occupancies[white], target_square)) {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    } else {
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    }
                    
                    pop_bit(attacks, target_square);
                }


                pop_bit(bitboard, source_square);
            }
        }

    }
}

U64 Board::hash_game_state() {
    U64 hash = 0xcbf29ce484222325;
    for (int piece = P; piece <= k; piece++) {
        hash ^= this->bitboards[piece] * 0x9e3779b97f4a7c15ULL;
    }
    if (this->side == white) {
        hash ^= 0xa5a5a5a5a5a5a5a5ULL;
    }
    if (this->enpassant != no_sq) {
        hash ^= 1ULL << (this->enpassant % 64);
    }
    hash ^= this->castle * 0xf0f0f0f0f0f0f0f0ULL;

    return hash;
}

void Board::update_repition_count(U64 board_hash) {
    this->repetition_count[board_hash]++;
};

int Board::get_repitition_count(U64 board_hash) {
    return this->repetition_count[board_hash] - 1;
}

std::tuple<int, int> Board::get_move_direction_and_distance(int source_square, int target_square) {
    int source_row = source_square / 8;
    int source_col = source_square % 8;
    int target_row = target_square / 8;
    int target_col = target_square % 8;

    int row_diff = target_row - source_row;
    int col_diff = target_col - source_col;

    if (row_diff == 0 && col_diff > 0) {
        return {(this->side == white) ? east : west, col_diff};
    } else if (row_diff == 0 && col_diff < 0) {
        return {(this->side == white) ? west : east, col_diff * -1};
    } else if (col_diff == 0 && row_diff > 0) {
        return {(this->side == white) ? south : north, row_diff};
    } else if (col_diff == 0 && row_diff < 0) {
        return {(this->side == white) ? north : south, row_diff * -1};
    } else if (row_diff > 0 && col_diff > 0) {
        return {(this->side == white) ? south_east : north_west, row_diff};
    } else if (row_diff > 0 && col_diff < 0) {
        return {(this->side == white) ? south_west : north_east, row_diff};
    } else if (row_diff < 0 && col_diff > 0) {
        return {(this->side == white) ? north_east : south_west, col_diff};
    } else if (row_diff < 0 && col_diff < 0) {
        return {(this->side == white) ? north_west : south_east, col_diff * -1};
    }
    return {-1, -1};
}

int Board::get_knight_move_direction(int source_square, int target_square) {
    int val = std::abs(source_square - target_square);

    if (val % 15 == 0) {
        if (val < 0) {
            return knight_sw;
        } else {
            return knight_ne;
        }
    } else if (val % 6 == 0) {
        if (val < 0) {
            return knight_ws;
        } else {
            return knight_en;
        }
    } else if (val % 10 == 0) {
        if (val < 0) {
            return knight_es;
        } else {
            return knight_wn;
        }
    } else if (val % 17 == 0) {
        if (val < 0) {
            return knight_se;
        } else {
            return knight_nw;
        }
    }
    return -1;
}

std::vector<std::vector<int>> rotate180(std::vector<std::vector<int>> board) {
    
    // Reverse each row
    for (auto& row : board) {
        std::reverse(row.begin(), row.end());
    }
    
    return board;
}

void Board::encode_board() {
    std::vector<std::vector<int>> new_state(14, std::vector<int>(64, 0));

    U64 board_hash = hash_game_state();
    update_repition_count(board_hash);

    for (int piece = P; piece <= k; piece++) {
        U64 bb = this->bitboards[piece];
        while (bb) {
            // int sq = __builtin_ctzll(bb);
            // int sq = ctzll(bb);
            int sq = get_ls1b_index(bb);
            // new_state[piece - start_piece][sq] = 1;
            new_state[piece][sq] = 1;
            bb &= bb - 1;
        }
    }
    int repitition_count = get_repitition_count(board_hash);
    this->n_repititions = repitition_count;
    new_state[12] = std::vector<int>(64, (this->side == white) ? repitition_count : this->prev_n_repititions); // white repitition layer
    new_state[13] = std::vector<int>(64, (this->side == black) ? repitition_count : this->prev_n_repititions); // black repitition layer

    this->current_state_pos = (this->current_state_pos + 1) % 8;
    if (this->state_history.size() >= 8) {
        this->state_history[this->current_state_pos] = new_state;
    } else {
        this->state_history.push_back(new_state);
    }
    this->prev_n_repititions = repitition_count;
    
}

std::vector<std::vector<int>> Board::get_ordered_state() {
    std::vector<std::vector<int>> ordered(119, std::vector<int>(64, 0));
    for (int t = 0; t < 8; t++) {
        int read_pos = (this->current_state_pos - t - 1 + 8) % 8;
        int dst_plane = t * 14;
        if (this->side == white) {
            if (read_pos < this->state_history.size()) {
                for (int piece = P; piece <= k; piece++) {
                    ordered[dst_plane + piece] = this->state_history[read_pos][piece];
                }
                // repitition layers
                ordered[dst_plane + k + 1] = this->state_history[read_pos][k + 1];
                ordered[dst_plane + k + 2] = this->state_history[read_pos][k + 2];
            }
            
        } else {
            if (read_pos < this->state_history.size()) {
                for (int piece = p; piece <= k; piece++){
                    ordered[dst_plane + piece - p] = this->state_history[read_pos][piece];
                }
                for (int piece = P; piece <= K; piece++){
                    ordered[dst_plane + piece + p] = this->state_history[read_pos][piece];
                }
                ordered[dst_plane + k + 2] = this->state_history[read_pos][k + 2];
                ordered[dst_plane + k + 1] = this->state_history[read_pos][k + 1];
            }
        }
    }
    // for (int t = 0; t < 16; t=t+2) {
    //     int read_pos = (this->current_state_pos - t - 2 + 16) % 16;
    //     int next_read_pos = (read_pos + 1) % 16;

    //     int dst_plane = t * 7;
    //     int next_dst_plane = (t + 1) * 7;

    //     for (int i = 0; i < 7; i++) {
    //         if (read_pos < this->state_history.size()) {
    //             ordered[dst_plane + i] = this->state_history[read_pos][i];
    //         }
    //         if (next_read_pos < this->state_history.size()) {
    //             ordered[next_dst_plane + i] = this->state_history[next_read_pos][i];
    //         }
    //     }
    // }
    (this->side == white) ? ordered[112] = std::vector<int>(64, white) : ordered[112] = std::vector<int>(64, black);
    ordered[113] = std::vector<int>(64, this->total_move_count);
    (this->castle & wk) ? ordered[114] = std::vector<int>(64, 1) : ordered[114] = std::vector<int>(64, 0);
    (this->castle & wq) ? ordered[115] = std::vector<int>(64, 1) : ordered[115] = std::vector<int>(64, 0);
    (this->castle & bk) ? ordered[116] = std::vector<int>(64, 1) : ordered[116] = std::vector<int>(64, 0);
    (this->castle & bq) ? ordered[117] = std::vector<int>(64, 1) : ordered[117] = std::vector<int>(64, 0);
    ordered[118] = std::vector<int>(64, this->no_progress_count);

    if (this->side == black && !this->has_restarted) {
        ordered = rotate180(ordered);
    }
    
    return ordered;
}

std::tuple<int, std::vector<std::vector<int>>, int, int> Board::reset() {
    this->has_restarted = 1;
    this->state_history.clear();
    this->repetition_count.clear();
    this->current_state_pos = 0;
    parse_fen(start_position);
    encode_board(); //white position
    std::vector<std::vector<int>> output_state = get_ordered_state();
    return {this->side, output_state, 0, 0}; //player, state, reward, terminal
}



std::vector<std::vector<int>> Board::get_legal_moves() {
    /*
    layer mapping

    0 - 55  : queen, rook, bishop, pawn, king moves. Each layer specifies direction and distance moved
    56 - 63 : knight moves for all 8 directions
    64 - 72 : under promotion moves (rook, bishop, knight) for pawns for all 3 directions NW, N, NE.
    */
    moves move_list[1];
    std::vector<std::vector<int>> actions(73, std::vector<int>(64, 0));
    this->move_index.clear();

    generate_moves(move_list);
    for (int move_count = 0; move_count < move_list->count; move_count++) {
        this->copy_board();
        int move = move_list->moves[move_count];
        if(!this->make_move(move, all_moves)) {
            continue;
        } else {
            this->take_back();
            int source_square = get_move_source(move);
            int target_square = get_move_target(move);
            int piece = get_move_piece(move);
            int promoted_piece = get_move_promoted(move);
            int capture = get_move_capture(move);
            int double_push = get_move_double(move);
            int enpass = get_move_enpassant(move);
            int castling = get_move_castling(move);

            if (piece == N || piece == n) {
                int direction = get_knight_move_direction(source_square, target_square);
                int layer = 56 + direction;
                actions[layer][source_square] = 1;
                this->move_index[(layer * 64) + source_square] = move;
            } else if (promoted_piece && (promoted_piece != Q || promoted_piece != q)) {
                auto [direction, _] = get_move_direction_and_distance(source_square, target_square);
                int layer = 64 + (direction * 3);
                if (promoted_piece == N || promoted_piece == n) {
                    actions[layer][source_square] = 1;
                    this->move_index[(layer * 64) + source_square] = move;
                } else if (promoted_piece == B || promoted_piece == b) {
                    actions[layer + 1][source_square] = 1;
                    this->move_index[((layer + 1) * 64) + source_square] = move;
                } else if (promoted_piece == R || promoted_piece == r) {
                    actions[layer + 2][source_square] = 1;
                    this->move_index[((layer + 2) * 64) + source_square] = move;
                }

            }
            else {
                auto [direction, distance] = get_move_direction_and_distance(source_square, target_square);
                int layer = direction * 7 + (distance - 1);
                actions[layer][source_square] = 1;
                this->move_index[(layer * 64) + source_square] = move;
            }
        }
    }
    return actions;
}

std::tuple<int, std::vector<std::vector<int>>, int, int> Board::step(int action_idx) {
    this->has_restarted = 0;
    int reward = 0;
    int terminal = 0;

    auto it = this->move_index.find(action_idx);
    if (it == this->move_index.end()) {
        throw std::invalid_argument("Invalid action");
    }
    this->make_move(this->move_index[action_idx], all_moves);
    encode_board();
    this->total_move_count++;
    std::vector<std::vector<int>> state = this->get_ordered_state();
    moves move_list[1];

    int in_check = is_square_attacked((this->side == white ? get_ls1b_index(this->bitboards[K]) : get_ls1b_index(this->bitboards[k])), this->side);
    int has_moves = 0;
    generate_moves(move_list);
    for (int move_count = 0; move_count < move_list->count; move_count++) {
        this->copy_board();
        int move = move_list->moves[move_count];
        if(!this->make_move(move, all_moves)) {
            continue;
        } else {
            has_moves = 1;
            this->take_back();
            break;
        }
    }

    // checkmate
    if (in_check && !has_moves) {
        (this->side == white) ? reward = -1 : reward = 1;
        terminal = 1;
    } else if (!in_check && !has_moves) {
        // stalemate
        reward = 0;
        terminal = 1;
    } else if (this->n_repititions == 2) {
        // 3 fold repitition draw
        reward = 0;
        terminal = 1;
    } else if (this->no_progress_count >= 50) {
        // 50 move draw
        reward = 0;
        terminal = 1;
    }

    return {this->side, state, reward, terminal};
}

/*
int total_move_count;
    int no_progress_count;
    std::unordered_map<U64, int> repetition_count;
    int n_repititions;
    int prev_n_repititions;
*/

State Board::save_state() {
    State state;
    memcpy(state.bitboards, this->bitboards, 96);
    memcpy(state.occupancies, this->occupancies, 24);
    state.side = this->side;
    state.enpassant = this->enpassant;
    state.castle = this->castle;
    state.state_history = this->state_history;
    state.state_pos = this->current_state_pos;
    state.no_progress_count = this->no_progress_count;
    state.repetition_count = this->repetition_count;
    state.n_repititions = this->n_repititions;
    state.prev_n_repititions = this->prev_n_repititions;
    state.move_index = this->move_index;

    return state;
}

std::tuple<int, std::vector<std::vector<int>>, int, int> Board::restore_state(State state) {
    memcpy(this->bitboards, state.bitboards, 96);
    memcpy(this->occupancies, state.occupancies, 24);
    this->side = state.side;
    this->enpassant = state.enpassant;
    this->castle = state.castle;
    this->state_history = state.state_history;
    this->current_state_pos = state.state_pos;
    this->no_progress_count = state.no_progress_count;
    this->repetition_count = state.repetition_count;
    this->n_repititions = state.n_repititions;
    this->prev_n_repititions = state.prev_n_repititions;
    this->move_index = state.move_index;

    std::vector<std::vector<int>> output_state = get_ordered_state();

    return {this->side, output_state, 0, 0}; // player, state, reward, terminal
}

int Board::roll_out() {
    std::random_device rd;
    std::mt19937 gen(rd());

    int terminal = 0;
    int reward = 0;
    int i = 0;

    while (!terminal && i < 400) {
        std::vector<std::vector<int>> actions = this->get_legal_moves();
        std::vector<int> flat_actions;
        for (const auto& row : actions) {
            flat_actions.insert(flat_actions.end(), row.begin(), row.end());
        }
        std::vector<int> legal_indices;
        for (int i = 0; i < flat_actions.size(); i++) {
            if (flat_actions[i] == 1) {
                legal_indices.push_back(i);
            }
        }
        std::uniform_int_distribution<> dis(0, legal_indices.size() - 1);
        int random_idx = dis(gen);
        int action_idx = legal_indices[random_idx];

        auto [p, s, r, t] = this->step(action_idx);
        terminal = t;
        reward = r;
        i++;
    }

    return reward;
}

unsigned int random_state = 1804289383;

U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};

U64 bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};

U64 get_bishop_attacks(int square, U64 occupancy) {
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];

    return bishop_attacks[square][occupancy];
}

U64 get_rook_attacks(int square, U64 occupancy) {
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];

    return rook_attacks[square][occupancy];
}

U64 get_queen_attacks(int square, U64 occupancy) {

    U64 queen_attacks = 0ULL;
    U64 bishop_occupancies = occupancy;
    U64 rook_occupancies = occupancy;

    bishop_occupancies &= bishop_masks[square];
    bishop_occupancies *= bishop_magic_numbers[square];
    bishop_occupancies >>= 64 - bishop_relevant_bits[square];

    queen_attacks = bishop_attacks[square][bishop_occupancies];

    rook_occupancies &= rook_masks[square];
    rook_occupancies *= rook_magic_numbers[square];
    rook_occupancies >>= 64 - rook_relevant_bits[square];

    queen_attacks |= rook_attacks[square][rook_occupancies];

    return queen_attacks;
}

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
    U64 occupancy = 0ULL;

    for (int count = 0; count < bits_in_mask; count++) {
        int square = get_ls1b_index(attack_mask);
        pop_bit(attack_mask, square);

        if (index & (1 << count)) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
};

U64 mask_pawn_attacks(int square, int side) {

    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    set_bit(bitboard, square);


    //white pawns
    if (!side) {
        attacks |= (bitboard >> 7) & not_a_file;
        attacks |= (bitboard >> 9) & not_h_file;

    } else {
        attacks |= (bitboard << 7) & not_h_file;
        attacks |= (bitboard << 9) & not_a_file;
    }

    return attacks;

};

U64 mask_knight_attacks(int square) {
    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    set_bit(bitboard, square);

    // if ((bitboard >> 6)) attacks |= (bitboard >> 6);
    if (bitboard & not_ab_file) {
        attacks |= (bitboard >> 6) & not_ab_file;
        attacks |= (bitboard >> 10);
        attacks |= (bitboard >> 15) & not_ab_file;
        attacks |= (bitboard >> 17);

        attacks |= (bitboard << 6);
        attacks |= (bitboard << 10) & not_ab_file;
        attacks |= (bitboard << 15);
        attacks |= (bitboard << 17) & not_ab_file;
    } else {
        attacks |= (bitboard >> 6);
        attacks |= (bitboard >> 10) & not_gh_file;
        attacks |= (bitboard >> 15);
        attacks |= (bitboard >> 17) & not_gh_file;

        attacks |= (bitboard << 6) & not_gh_file;
        attacks |= (bitboard << 10);
        attacks |= (bitboard << 15) & not_gh_file;
        attacks |= (bitboard << 17);
    }
    

    return attacks;
}

U64 mask_king_attacks(int square) {
    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    set_bit(bitboard, square);
    if (bitboard & not_a_file) {
        attacks |= (bitboard << 1) & not_a_file;
        attacks |= (bitboard >> 1);
        attacks |= (bitboard >> 8);
        attacks |= (bitboard >> 7) & not_a_file;
        attacks |= (bitboard >> 9);
        attacks |= (bitboard << 8);
        attacks |= (bitboard << 7);
        attacks |= (bitboard << 9) & not_a_file;
    } else {
        attacks |= (bitboard << 1);
        attacks |= (bitboard >> 1) & not_h_file;
        attacks |= (bitboard >> 8);
        attacks |= (bitboard >> 7);
        attacks |= (bitboard >> 9) & not_h_file;
        attacks |= (bitboard << 8);
        attacks |= (bitboard << 7) & not_h_file;
        attacks |= (bitboard << 9);
    }
    return attacks;

}

U64 mask_bishop_attacks(int square) {
    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    set_bit(bitboard, square);

    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

    return attacks;
}

U64 mask_rook_attacks(int square) {
    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    set_bit(bitboard, square);

    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

    return attacks;
}

U64 bishop_attacks_on_fly(int square, U64 block) {
    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    set_bit(bitboard, square);

    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) {
            break;
        }
    } 
    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) {
            break;
        }
    };
    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) {
            break;
        }
    };
    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) {
            break;
        }
    };

    return attacks;
}

U64 rook_attacks_on_fly(int square, U64 block) {
    U64 bitboard = 0ULL;
    U64 attacks = 0ULL;

    set_bit(bitboard, square);

    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r = tr + 1; r <= 7; r++) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block) {
            break;
        }
    };
    for (r = tr - 1; r >= 0; r--) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block) {
            break;
        }
    };
    for (f = tf + 1; f <= 7; f++) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block) {
            break;
        }
    };
    for (f = tf - 1; f >= 0; f--) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block) {
            break;
        }
    };

    return attacks;
}

void init_leaper_attacks() {

    for (int square = 0; square < 64; square++) {
        pawn_attacks[white][square] = mask_pawn_attacks(square, white);
        pawn_attacks[black][square] = mask_pawn_attacks(square, black);

        knight_attacks[square] = mask_knight_attacks(square);

        king_attacks[square] = mask_king_attacks(square);
    }
};

unsigned int get_random_U32_number() {
    unsigned int number = random_state;

    //XOR shift alg
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    random_state = number;

    return number;
};

U64 get_random_U64_number() {
    U64 n1, n2, n3, n4;

    n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(get_random_U32_number()) & 0xFFFF;

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
};

U64 generate_magic_number() {
    return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}

U64 find_magic_number(int square, int relevant_bits, int bishop) {
    U64 occupancies[4096];
    U64 attacks[4096];
    U64 used_attacks[4096];
    U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);

    int occupancy_indicies = 1 << relevant_bits;

    for (int index = 0; index < occupancy_indicies; index++) {
        occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);
        attacks[index] = bishop ? bishop_attacks_on_fly(square, occupancies[index]) : rook_attacks_on_fly(square, occupancies[index]);
    };

    for (int random_count = 0; random_count < 100000000; random_count++) {
        U64 magic_number = generate_magic_number();
        if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6) {
            continue;
        } else {
            memset(used_attacks, 0ULL, sizeof(used_attacks));

            int index, fail;

            for (index = 0, fail = 0; !fail && index < occupancy_indicies; index++) {
                int magic_index = (int)((occupancies[index] * magic_number) >> (64 - relevant_bits));

                // if magic index works:
                if (used_attacks[magic_index] == 0ULL) {
                    used_attacks[magic_index] = attacks[index];
                } else if (used_attacks[magic_index] != attacks[index]) {
                    // magic index doesn't work
                    fail = 1;
                }
            }

            if (!fail) {
                return magic_number;
            }
        }

    };
    printf("Magic number failed");
    return 0ULL;
};

void init_magic_numbers() {
    for (int square = 0; square < 64; square++) {
        // rook magic numbers
        rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);
    };
    printf("\n\n");
    for (int square = 0; square < 64; square++) {
        // bishop magic numbers
        bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_bits[square], bishop);
    }
};

void init_sliders_attacks(int bishop) {
    for (int square = 0; square < 64; square++) {
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
        int relevant_bits_count = count_bits(attack_mask);
        int occupancy_indicies = (1 << relevant_bits_count);

        for (int index = 0; index < occupancy_indicies; index++) {
            if (bishop) {
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);
                bishop_attacks[square][magic_index] = bishop_attacks_on_fly(square, occupancy);
            } else {
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);
                rook_attacks[square][magic_index] = rook_attacks_on_fly(square, occupancy);
            }
        }
    }
}