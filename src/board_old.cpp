#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "board_old.h"
#include "move_tables.h"


namespace py = pybind11;
// U64 bitboards[12];
// U64 occupancies[3];
// int side = -1;
// int enpassant = no_sq;
// int castle;
// int no_progress_count = 0;
// int current_state_pos = 0;
// int total_move_count = 0;

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
        copy_board();

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
            take_back();
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
    return this->repetition_count[board_hash];
}

std::tuple<int, int> Board::get_move_direction_and_distance(int source_square, int target_square) {
    int source_row = source_square / 8;
    int source_col = source_square % 8;
    int target_row = target_square / 8;
    int target_col = target_square % 8;

    int row_diff = target_row - source_row;
    int col_diff = target_col - source_col;

    if (row_diff == 0 && col_diff > 0) {
        return {east, col_diff};
    } else if (row_diff == 0 && col_diff < 0) {
        return {west, col_diff * -1};
    } else if (col_diff == 0 && row_diff > 0) {
        return {south, row_diff};
    } else if (col_diff == 0 && row_diff < 0) {
        return {north, row_diff * -1};
    } else if (row_diff > 0 && col_diff > 0) {
        return {south_east, row_diff};
    } else if (row_diff > 0 && col_diff < 0) {
        return {south_west, row_diff};
    } else if (row_diff < 0 && col_diff > 0) {
        return {north_east, col_diff};
    } else if (row_diff < 0 && col_diff < 0) {
        return {north_west, col_diff * -1};
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

std::vector<std::vector<int>> Board::encode_board(int player) {
    std::vector<std::vector<int>> new_state(7, std::vector<int>(64, 0));

    U64 board_hash = hash_game_state();
    update_repition_count(board_hash);


    int start_piece = (player == white) ? P : p;
    int end_piece = (player == white) ? K : k;
    

    for (int piece = start_piece; piece <= end_piece; piece++) {
        U64 bb = this->bitboards[piece];
        while (bb) {
            // int sq = __builtin_ctzll(bb);
            // int sq = ctzll(bb);
            int sq = get_ls1b_index(bb);
            new_state[piece - start_piece][sq] = 1;
            bb &= bb - 1;
        }
    }
    int repitition_count = get_repitition_count(board_hash);
    this->n_repititions = repitition_count;
    new_state[6] = std::vector<int>(64, repitition_count);

    return new_state;
}

void Board::update_state(int player) {
    std::vector<std::vector<int>> new_state = encode_board(player);
    
    // Write directly to circular buffer position
    int write_plane = this->current_state_pos * 7;
    for (int i = 0; i < 7; i++) {
        this->state[write_plane + i] = new_state[i];
    }
    
    // Advance position
    this->current_state_pos = (this->current_state_pos + 1) % 16;
}

std::vector<std::vector<int>> Board::get_ordered_state() {
    std::vector<std::vector<int>> ordered(119, std::vector<int>(64, 0));
    
    for (int t = 0; t < 16; t++) {
        int read_pos = (this->current_state_pos - 1 - t + 16) % 16;
        
        int src_plane = read_pos * 7;
        int dst_plane = t * 7;
        
        for (int i = 0; i < 7; i++) {
            ordered[dst_plane + i] = this->state[src_plane + i];
        }
    }
    (this->side == white) ? ordered[112] = std::vector<int>(64, white) : ordered[112] = std::vector<int>(64, black);
    ordered[113] = std::vector<int>(64, this->total_move_count);
    (this->castle & wk) ? ordered[114] = std::vector<int>(64, 1) : ordered[114] = std::vector<int>(64, 0);
    (this->castle & wq) ? ordered[115] = std::vector<int>(64, 1) : ordered[115] = std::vector<int>(64, 0);
    (this->castle & bk) ? ordered[116] = std::vector<int>(64, 1) : ordered[116] = std::vector<int>(64, 0);
    (this->castle & bq) ? ordered[117] = std::vector<int>(64, 1) : ordered[117] = std::vector<int>(64, 0);
    ordered[118] = std::vector<int>(64, this->no_progress_count);
    
    return ordered;
}

// std::tuple<std::vector<std::vector<int>>, int, int> Board::reset() {
//     this->state = std::vector<std::vector<int>>(119, std::vector<int>(64, 0));
//     this->current_state_pos = 0;
//     parse_fen(start_position);
//     update_state(white); //white position
//     update_state(black); // black position
//     std::vector<std::vector<int>> output_state = get_ordered_state();
//     return {output_state, 0, 0}; // state, reward, terminal
// }

py::tuple Board::reset() {
    this->state = std::vector<std::vector<int>>(119, std::vector<int>(64, 0));
    this->current_state_pos = 0;
    parse_fen(start_position);
    this->update_state(white); //white position
    this->update_state(black); // black position
    std::vector<std::vector<int>> output_state = get_ordered_state();
    return py::make_tuple(std::move(output_state), 0, 0); // state, reward, terminal
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
        copy_board();
        int move = move_list->moves[move_count];
        if(!this->make_move(move, all_moves)) {
            continue;
        } else {
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
            } else if (promoted_piece) {
                auto [direction, _] = get_move_direction_and_distance(source_square, target_square);
                int layer = 64 + (direction * 3);
                if (promoted_piece == N || promoted_piece == n) {
                    actions[layer][source_square] = 1;
                    this->move_index[(layer * 64) + source_square] = move;
                } else if (promoted_piece == B || promoted_piece == b) {
                    actions[layer + 1][source_square] = 1;
                    this->move_index[((layer * 1) * 64) + source_square] = move;
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
        take_back();
    }
    return actions;
}

py::tuple Board::step(int action_idx) {
    int reward = 0;
    int terminal = 0;

    auto it = this->move_index.find(action_idx);
    if (it == this->move_index.end()) {
        throw std::invalid_argument("Invalid action");
    }
    this->make_move(this->move_index[action_idx], all_moves);
    update_state((this->side == white) ? black : white);
    std::vector<std::vector<int>> state = this->get_ordered_state();
    moves move_list[1];

    int in_check = is_square_attacked((this->side == white ? get_ls1b_index(this->bitboards[K]) : get_ls1b_index(this->bitboards[k])), this->side);
    int has_moves = 0;
    generate_moves(move_list);
    for (int move_count = 0; move_count < move_list->count; move_count++) {
        copy_board();
        int move = move_list->moves[move_count];
        if(!this->make_move(move, all_moves)) {
            continue;
        } else {
            has_moves = 1;
            take_back();
            break;
        }
    }

    // checkmate
    if (in_check && !has_moves) {
        (this->side == white) ? reward = -1 : reward = 1;
        terminal = 1;
    } else if (!in_check && !has_moves) {
        reward = 0;
        terminal = 1;
    } else if (this->n_repititions == 3) {
        reward = 0;
        terminal = 1;
    } else if (this->no_progress_count >= 50) {
        reward = 0;
        terminal = 1;
    }
    return py::make_tuple(std::move(state), reward, terminal);
}