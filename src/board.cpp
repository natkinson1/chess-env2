#include "board.h"
#include "move_tables.h"

std::vector<std::vector<int>> state(119, std::vector<int>(64, 0));
U64 bitboards[12];
U64 occupancies[3];
int side = -1;
int enpassant = no_sq;
int castle;
int no_progress_count = 0;
int current_state_pos = 0;
int total_move_count = 0;
std::unordered_map<U64, int> repetition_count;

void print_board() {
    printf("\n");
    for (int rank = 0; rank < 8; rank++) {
        printf(" %d|", 8 - rank);
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            int piece = -1;
            for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                if (get_bit(bitboards[bb_piece], square)) {
                    piece = bb_piece;
                }
            }

            printf("  %c", (piece == -1) ? '.' : ascii_pieces[piece]);
        }
        printf("\n");
    }
    printf("   ------------------------");
    printf("\n");
    printf("     a  b  c  d  e  f  g  h");
    printf("\n\n");
    printf("Side:       %s\n", !side ? "white" : "black");
    printf("enpassant:  %s\n", (enpassant != no_sq) ? square_to_coord[enpassant] : "NA");
    printf("Castling:   %c%c%c%c\n\n\n", (castle & wk) ? 'K' : '-', 
                                     (castle & wq) ? 'Q' : '-', 
                                     (castle & bk) ? 'k' : '-', 
                                     (castle & bq) ? 'q' : '-');

}

void parse_fen(const char *fen) {
    memset(bitboards, 0ULL, 96);
    memset(occupancies, 0ULL, 24);
    side = 0;
    enpassant = no_sq;
    castle = 0;

    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            if ((*fen >= 'a' && *fen <= 'z') | (*fen >= 'A' && *fen <= 'Z')) {
                int piece = char_to_piece(*fen);
                set_bit(bitboards[piece], square);

                fen++;
            }

            if (*fen >= '0' && *fen <= '9') {
                int offset = *fen - '0'; // convert char 0 -> int 0
                int piece = -1;
                // if piece on current square -> get piece code
                for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                    if (get_bit(bitboards[bb_piece], square)) {
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
    (*fen == 'w') ? (side = white) : (side = black);
    
    // castling rights
    fen += 2;
    while (*fen != ' ') {
        switch (*fen) {
            case 'K': 
                castle |= wk;
                break;
            case 'k': 
                castle |= bk;
                break;
            case 'Q': 
                castle |= wq;
                break;
            case 'q': 
                castle |= bq;
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
        enpassant = rank * 8 + file;
    } else {
        enpassant = no_sq;
    }

    // loop over white pieces
    for (int piece = P; piece <= K; piece++) {
        occupancies[white] |= bitboards[piece];
    }
    // black pieces
    for (int piece = p; piece <= k; piece++) {
        occupancies[black] |= bitboards[piece];
    }

    // both
    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];

};

static inline int is_square_attacked(int square, int side) {

    if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) {
        return 1;
    }
    // black pawns
    if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) {
        return 1;
    }
    // knight attacks
    if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) {
        return 1;
    }
    if (get_bishop_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) {
        return 1;
    }
    if (get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) {
        return 1;
    }
    if (get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) {
        return 1;
    }
    // king attacks
    if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) {
        return 1;
    }

    return 0;
}

static inline void add_move(moves  *move_list, int move) {
    // store move
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

static inline int make_move(int move, int move_flag) {

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
            no_progress_count = 0;
        }
        
        pop_bit(bitboards[piece], source_square);
        set_bit(bitboards[piece], target_square);

        if (capture) {
            no_progress_count = 0;
            int start_piece, end_piece;
            if (side == white) {
                start_piece = p;
                end_piece = k;
            } else {
                start_piece = P;
                end_piece = K;
            }
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) {
                if (get_bit(bitboards[bb_piece], target_square)) {
                    pop_bit(bitboards[bb_piece], target_square);
                    break;
                }
            }
        }
        // pawn promotions
        if (promoted_piece) {
            no_progress_count = 0;
            pop_bit(bitboards[(side == white) ? P : p], target_square);
            set_bit(bitboards[promoted_piece], target_square);
        }

        if (enpass) {
            no_progress_count = 0;
            (side == white) ? pop_bit(bitboards[p], target_square + 8) : pop_bit(bitboards[P], target_square - 8);
        }
        enpassant = no_sq;

        if (double_push) {
            no_progress_count = 0;
            (side == white) ? (enpassant = target_square + 8) : (enpassant = target_square - 8);
        }

        if (castling) {
            switch (target_square) {
                // move respective rook.
                case (g1):
                    pop_bit(bitboards[R], h1);
                    set_bit(bitboards[R], f1);
                    break;
                case (c1):
                    pop_bit(bitboards[R], a1);
                    set_bit(bitboards[R], d1);
                    break;
                case (g8):
                    pop_bit(bitboards[r], h8);
                    set_bit(bitboards[r], f8);
                    break;
                case (c8):
                    pop_bit(bitboards[r], a8);
                    set_bit(bitboards[r], d8);
                    break;
            }
        }
        // update castling rights
        castle &= castling_rights[source_square];
        castle &= castling_rights[target_square];

        // update occupancies
        memset(occupancies, 0ULL, 24);
        
        for (int bb_piece = P; bb_piece <= K; bb_piece++) {
            occupancies[white] |= bitboards[bb_piece];
        };
        for (int bb_piece = p; bb_piece <= k; bb_piece++) {
            occupancies[black] |= bitboards[bb_piece];
        }
        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        // change side
        side ^= 1;
        no_progress_count++;

        // ensure king isn't in check
        if (is_square_attacked((side == white ? get_ls1b_index(bitboards[k]) : get_ls1b_index(bitboards[K])), side)) {
            // move is illegal
            no_progress_count--;
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

static inline void generate_moves(moves *move_list) {

    move_list->count = 0;

    int source_square, target_square;

    U64 bitboard, attacks;

    for (int piece = P; piece <= k; piece++) {
        bitboard = bitboards[piece];

        // generate white pawn and white king castling
        if (side == white) {
            if (piece == P) {
                while (bitboard) {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square - 8;

                    if (!(target_square < a8) && !get_bit(occupancies[both], target_square)) {
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
                            if ((source_square >= a2 && source_square <= h2) && !get_bit(occupancies[both], target_square - 8)) {
                                add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                            }

                        }
                    }
                    attacks = pawn_attacks[side][source_square] & occupancies[black];

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
                    if (enpassant != no_sq) {
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks) {
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    pop_bit(bitboard, source_square);
                }
            }
            if (piece == K) {
                // king side castling available
                if (castle & wk) {
                    // ensure clear path
                    if (!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1)) {
                        // doesn't move through check
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black)) {
                            add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }

                // queen side castling available
                if (castle & wq) {
                    if (!get_bit(occupancies[both], d1) && !get_bit(occupancies[both], c1) && !get_bit(occupancies[both], b1)) {
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

                    if (!(target_square > h1) && !get_bit(occupancies[both], target_square)) {
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
                            if ((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], target_square + 8)) {
                                add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                            }

                        }
                        
                    }
                    attacks = pawn_attacks[side][source_square] & occupancies[white];

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
                    if (enpassant != no_sq) {
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks) {
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    pop_bit(bitboard, source_square);
                }
            }
            if (piece == k) {
                // king side castling available
                if (castle & bk) {
                    // ensure clear path
                    if (!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8)) {
                        // doesn't move through check
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white)) {
                            add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }

                // queen side castling available
                if (castle & bq) {
                    if (!get_bit(occupancies[both], d8) && !get_bit(occupancies[both], c8) && !get_bit(occupancies[both], b8)) {
                        // doesn't move through check
                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white)) {
                            add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }

        // generate knight moves
        if ((side == white) ? piece == N : piece == n) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((side == white) ? occupancies[black] : occupancies[white], target_square)) {
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
        if ((side == white) ? piece == B : piece == b) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = get_bishop_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((side == white) ? occupancies[black] : occupancies[white], target_square)) {
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
        if ((side == white) ? piece == R : piece == r) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((side == white) ? occupancies[black] : occupancies[white], target_square)) {
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
        if ((side == white) ? piece == Q : piece == q) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((side == white) ? occupancies[black] : occupancies[white], target_square)) {
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
        if ((side == white) ? piece == K : piece == k) {
            while (bitboard) {
                int source_square = get_ls1b_index(bitboard);
                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks) {
                    int target_square = get_ls1b_index(attacks);
                    if (!get_bit((side == white) ? occupancies[black] : occupancies[white], target_square)) {
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

U64 hash_game_state() {
    U64 hash = 0xcbf29ce484222325;
    for (int piece = P; piece <= k; piece++) {
        hash ^= bitboards[piece] * 0x9e3779b97f4a7c15ULL;
    }
    if (side == white) {
        hash ^= 0xa5a5a5a5a5a5a5a5ULL;
    }
    if (enpassant != no_sq) {
        hash ^= 1ULL << (enpassant % 64);
    }
    hash ^= castle * 0xf0f0f0f0f0f0f0f0ULL;

    return hash;
}

void update_repition_count(U64 board_hash) {
    repetition_count[board_hash]++;
};

int get_repitition_count(U64 board_hash) {
    return repetition_count[board_hash];
}

std::tuple<int, int> get_move_direction_and_distance(int source_square, int target_square) {
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

int get_knight_move_direction(int source_square, int target_square) {
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

std::vector<std::vector<int>> encode_board() {
    std::vector<std::vector<int>> new_state(7, std::vector<int>(64, 0));

    U64 board_hash = hash_game_state();
    update_repition_count(board_hash);


    int start_piece = (side == white) ? P : p;
    int end_piece = (side == white) ? K : k;
    

    for (int piece = start_piece; piece <= end_piece; piece++) {
        U64 bb = bitboards[piece];
        while (bb) {
            // int sq = __builtin_ctzll(bb);
            // int sq = ctzll(bb);
            int sq = get_ls1b_index(bb);
            new_state[piece][sq] = 1;
            bb &= bb - 1;
        }
    }
    int repitition_count = get_repitition_count(board_hash);
    new_state[6] = std::vector<int>(64, repitition_count);

    return new_state;
}

void update_state() {
    std::vector<std::vector<int>> new_state = encode_board();
    
    // Write directly to circular buffer position
    int write_plane = current_state_pos * 7;
    for (int i = 0; i < 7; i++) {
        state[write_plane + i] = new_state[i];
    }
    
    // Advance position
    current_state_pos = (current_state_pos + 1) % 16;
}

std::vector<std::vector<int>> get_ordered_state() {
    std::vector<std::vector<int>> ordered(119, std::vector<int>(64, 0));
    
    for (int t = 0; t < 16; t++) {
        // Calculate which time step to read
        // current_pos points to where NEXT write will go
        // So current_pos - 1 is the newest data
        int read_pos = (current_state_pos - 1 - t + 16) % 16;
        
        int src_plane = read_pos * 7;
        int dst_plane = t * 7;
        
        // Copy 7 planes for this time step
        for (int i = 0; i < 7; i++) {
            ordered[dst_plane + i] = state[src_plane + i];
        }
    }
    (side == white) ? ordered[112] = std::vector<int>(64, white) : ordered[112] = std::vector<int>(64, black);
    ordered[113] = std::vector<int>(64, total_move_count);
    (castle & wk) ? ordered[114] = std::vector<int>(64, 1) : ordered[114] = std::vector<int>(64, 0);
    (castle & wq) ? ordered[115] = std::vector<int>(64, 1) : ordered[115] = std::vector<int>(64, 0);
    (castle & bk) ? ordered[116] = std::vector<int>(64, 1) : ordered[116] = std::vector<int>(64, 0);
    (castle & bq) ? ordered[117] = std::vector<int>(64, 1) : ordered[117] = std::vector<int>(64, 0);
    ordered[118] = std::vector<int>(64, no_progress_count);
    
    return ordered;
}

std::vector<std::vector<int>> reset() {
    state = std::vector<std::vector<int>>(119, std::vector<int>(64, 0));
    parse_fen(start_position);
    update_state(); //white position
    update_state(); // black position
    std::vector<std::vector<int>> output_state = get_ordered_state();
    return output_state;
}