/*
	Shortest Draw Othello

	@file Shortest_Draw_Othello.cpp
		Main file
	@date 2024
	@author Takuto Yamana
	@license GPL-3.0 license
*/

#include <iostream>
#include "engine/board.hpp"
#include "engine/util.hpp"


void init(){
    bit_init();
    mobility_init();
    flip_init();
}

void output_transcript(std::vector<int> &transcript){
    for (int &move: transcript){
        std::cout << idx_to_coord(move);
    }
    std::cout << std::endl;
}

/***** from https://github.com/Nyanyan/Reverse_Othello start *****/
inline uint64_t full_stability_h(uint64_t full){
    full &= full >> 1;
    full &= full >> 2;
    full &= full >> 4;
    return (full & 0x0101010101010101ULL) * 0xFF;
}

inline uint64_t full_stability_v(uint64_t full){
    full &= (full >> 8) | (full << 56);
    full &= (full >> 16) | (full << 48);
    full &= (full >> 32) | (full << 32);
    return full;
}

inline void full_stability_d(uint64_t full, uint64_t *full_d7, uint64_t *full_d9){
    constexpr uint64_t edge = 0xFF818181818181FFULL;
    uint64_t l7, r7, l9, r9;
    l7 = r7 = full;
    l7 &= edge | (l7 >> 7);		r7 &= edge | (r7 << 7);
    l7 &= 0xFFFF030303030303ULL | (l7 >> 14);	r7 &= 0xC0C0C0C0C0C0FFFFULL | (r7 << 14);
    l7 &= 0xFFFFFFFF0F0F0F0FULL | (l7 >> 28);	r7 &= 0xF0F0F0F0FFFFFFFFULL | (r7 << 28);
    *full_d7 = l7 & r7;

    l9 = r9 = full;
    l9 &= edge | (l9 >> 9);		r9 &= edge | (r9 << 9);
    l9 &= 0xFFFFC0C0C0C0C0C0ULL | (l9 >> 18);	r9 &= 0x030303030303FFFFULL | (r9 << 18);
    *full_d9 = l9 & r9 & (0x0F0F0F0FF0F0F0F0ULL | (l9 >> 36) | (r9 << 36));
}

inline void full_stability(uint64_t discs, uint64_t *h, uint64_t *v, uint64_t *d7, uint64_t *d9){
    *h = full_stability_h(discs);
    *v = full_stability_v(discs);
    full_stability_d(discs, d7, d9);
}

inline uint64_t enhanced_stability(Board *board, const uint64_t goal_mask){
    uint64_t full_h, full_v, full_d7, full_d9;
    uint64_t discs = board->player | board->opponent;
    full_stability(discs | ~goal_mask, &full_h, &full_v, &full_d7, &full_d9);
    full_h &= goal_mask;
    full_v &= goal_mask;
    full_d7 &= goal_mask;
    full_d9 &= goal_mask;
    uint64_t h, v, d7, d9;
    uint64_t stability = 0ULL, n_stability;
    n_stability = discs & (full_h & full_v & full_d7 & full_d9);
    while (n_stability & ~stability){
        stability |= n_stability;
        h = (stability >> 1) | (stability << 1) | full_h;
        v = (stability >> 8) | (stability << 8) | full_v;
        d7 = (stability >> 7) | (stability << 7) | full_d7;
        d9 = (stability >> 9) | (stability << 9) | full_d9;
        n_stability = h & v & d7 & d9;
    }
    return stability;
}

void find_path_p(Board *board, std::vector<int> &path, int player, const uint64_t goal_mask, const uint64_t corner_mask, const int goal_n_discs, const Board *goal, const int goal_player, uint64_t *n_nodes, uint64_t *n_solutions){
    ++(*n_nodes);
    if (player == goal_player && board->player == goal->player && board->opponent == goal->opponent){
        output_transcript(path);
        ++(*n_solutions);
        return;
    }
    uint64_t goal_board_player, goal_board_opponent;
    if (player != goal_player){
        goal_board_player = goal->opponent;
        goal_board_opponent = goal->player;
    } else{
        goal_board_player = goal->player;
        goal_board_opponent = goal->opponent;
    }
    uint64_t stable = enhanced_stability(board, goal_mask);
    if ((stable & board->player & goal_board_opponent) || (stable & board->opponent & goal_board_player))
        return;
    uint64_t legal = board->get_legal() & goal_mask & ~(corner_mask & goal_board_opponent);
    if (legal){
        Flip flip;
        for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){;
            calc_flip(&flip, board, cell);
            board->move_board(&flip);
            path.emplace_back(cell);
                find_path_p(board, path, player ^ 1, goal_mask, corner_mask, goal_n_discs, goal, goal_player, n_nodes, n_solutions);
            path.pop_back();
            board->undo_board(&flip);
        }
    }
}

// find path to the given board
void find_path(Board *goal, uint64_t *n_solutions){
    uint64_t goal_mask = goal->player | goal->opponent; // legal candidate
    uint64_t corner_mask = 0ULL; // cells that work as corner (non-flippable cells)
    uint64_t empty_mask_r1 = ((~goal_mask & 0xFEFEFEFEFEFEFEFEULL) >> 1) | 0x8080808080808080ULL;
    uint64_t empty_mask_l1 = ((~goal_mask & 0x7F7F7F7F7F7F7F7FULL) << 1) | 0x0101010101010101ULL;
    uint64_t empty_mask_r8 = ((~goal_mask & 0xFFFFFFFFFFFFFF00ULL) >> 8) | 0xFF00000000000000ULL;
    uint64_t empty_mask_l8 = ((~goal_mask & 0x00FFFFFFFFFFFFFFULL) << 8) | 0x00000000000000FFULL;
    uint64_t empty_mask_r7 = ((~goal_mask & 0x7F7F7F7F7F7F7F00ULL) >> 7) | 0xFF01010101010101ULL;
    uint64_t empty_mask_l7 = ((~goal_mask & 0x00FEFEFEFEFEFEFEULL) << 7) | 0x80808080808080FFULL;
    uint64_t empty_mask_r9 = ((~goal_mask & 0xFEFEFEFEFEFEFE00ULL) >> 9) | 0x01010101010101FFULL;
    uint64_t empty_mask_l9 = ((~goal_mask & 0x007F7F7F7F7F7F7FULL) << 9) | 0xFF80808080808080ULL;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_r8 & empty_mask_l9 & empty_mask_l7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_r1 & empty_mask_l8 & empty_mask_l9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_r8 & empty_mask_l9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_r9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_r9 & empty_mask_l7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_l9 & empty_mask_r7;
    corner_mask |= empty_mask_l1 & empty_mask_l8 & empty_mask_l9 & empty_mask_l7;
    corner_mask &= goal_mask;

    //bit_print_board(goal_mask);
    //bit_print_board(corner_mask);

    int n_discs = pop_count_ull(goal_mask);
    Board board = {0x0000000810000000ULL, 0x0000001008000000ULL};
    std::vector<int> path;
    uint64_t strt = tim();
    uint64_t n_nodes = 0;
    find_path_p(&board, path, BLACK, goal_mask, corner_mask, n_discs, goal, BLACK, &n_nodes, n_solutions);
    find_path_p(&board, path, BLACK, goal_mask, corner_mask, n_discs, goal, WHITE, &n_nodes, n_solutions);
    uint64_t elapsed = tim() - strt;
    //std::cout << "found " << n_solutions << " solutions in " << elapsed << " ms " << n_nodes << " nodes" << std::endl;
    //std::cerr << "found " << n_solutions << " solutions in " << elapsed << " ms " << n_nodes << " nodes" << std::endl;
}
/***** from https://github.com/Nyanyan/Reverse_Othello end *****/


// generate draw endgame
// nCr(24, 12) = 2.7e6 <= small enough to generate all boards and check game over
void generate_check_boards(Board *board, uint64_t discs, int n_discs_half, uint64_t *n_solutions){
    if (discs == 0){
        if (board->is_end()){ // game over
            //std::cerr << board->n_discs() << " " << board->score_player() << std::endl;
            //board->print();
            find_path(board, n_solutions);
            //++(*n_solutions);
        }
        return;
    }
    if (pop_count_ull(board->player) == n_discs_half){
        board->opponent ^= discs;
            generate_check_boards(board, 0, n_discs_half, n_solutions);
        board->opponent ^= discs;
    } else if (pop_count_ull(board->opponent) == n_discs_half){
        board->player ^= discs;
            generate_check_boards(board, 0, n_discs_half, n_solutions);
        board->player ^= discs;
    } else{
        uint64_t cell_bit = 1ULL << ctz(discs);
        discs ^= cell_bit;
        board->player ^= cell_bit;
            generate_check_boards(board, discs, n_discs_half, n_solutions);
        board->player ^= cell_bit;
        board->opponent ^= cell_bit;
            generate_check_boards(board, discs, n_discs_half, n_solutions);
        board->opponent ^= cell_bit;
    }
}

// generate silhouettes
void find_draw(uint64_t discs, int depth, uint64_t put_cells, uint64_t *n_solutions){
    if (depth == 0){
        uint64_t visited = 0;
        uint64_t n_visited = 1ULL << ctz(discs);
        while (visited != n_visited){
            visited = n_visited;
            n_visited |= (visited & 0x7F7F7F7F7F7F7F7FULL) << 1;
            n_visited |= (visited & 0xFEFEFEFEFEFEFEFEULL) >> 1;
            n_visited |= (visited & 0x00FFFFFFFFFFFFFFULL) << 8;
            n_visited |= (visited & 0xFFFFFFFFFFFFFF00ULL) >> 8;
            n_visited |= (visited & 0x00FEFEFEFEFEFEFEULL) << 7;
            n_visited |= (visited & 0x7F7F7F7F7F7F7F00ULL) >> 7;
            n_visited |= (visited & 0x007F7F7F7F7F7F7FULL) << 9;
            n_visited |= (visited & 0xFEFEFEFEFEFEFE00ULL) >> 9;
            n_visited &= discs;
        }
        if (visited == discs){ // all discs are connected
            Board board;
            board.player = 0;
            board.opponent = 0;
            generate_check_boards(&board, discs, pop_count_ull(discs) / 2, n_solutions);
        }
        return;
    }
    uint64_t neighbours = 0;
    neighbours |= ((discs & 0x7F7F7F7F7F7F7F7FULL) << 1) & ((discs & 0x3F3F3F3F3F3F3F3FULL) << 2);
    neighbours |= ((discs & 0xFEFEFEFEFEFEFEFEULL) >> 1) & ((discs & 0xFCFCFCFCFCFCFCFCULL) >> 2);
    neighbours |= ((discs & 0x00FFFFFFFFFFFFFFULL) << 8) & ((discs & 0x0000FFFFFFFFFFFFULL) << 16);
    neighbours |= ((discs & 0xFFFFFFFFFFFFFF00ULL) >> 8) & ((discs & 0xFFFFFFFFFFFF0000ULL) >> 16);
    neighbours |= ((discs & 0x00FEFEFEFEFEFEFEULL) << 7) & ((discs & 0x0000FCFCFCFCFCFCULL) << 14);
    neighbours |= ((discs & 0x7F7F7F7F7F7F7F00ULL) >> 7) & ((discs & 0x3F3F3F3F3F3F0000ULL) >> 14);
    neighbours |= ((discs & 0x007F7F7F7F7F7F7FULL) << 9) & ((discs & 0x00003F3F3F3F3F3FULL) << 18);
    neighbours |= ((discs & 0xFEFEFEFEFEFEFE00ULL) >> 9) & ((discs & 0xFCFCFCFCFCFC0000ULL) >> 18);
    neighbours &= ~discs;
    neighbours &= ~put_cells;
    if (neighbours){
        for (uint_fast8_t cell = first_bit(&neighbours); neighbours; cell = next_bit(&neighbours)){
            put_cells ^= 1ULL << cell;
            discs ^= 1ULL << cell;
                find_draw(discs, depth - 1, put_cells, n_solutions);
            discs ^= 1ULL << cell;
        }
    }
}

int main(int argc, char* argv[]){
    init();

    // need 1 or more full line to cause gameover by draw  (because there are both black and white discs)
    std::vector<uint64_t> conditions = {
        // horizontal
        0x00000000000000FFULL,
        0x000000000000FF00ULL,
        0x0000000000FF0000ULL,
        0x00000000FF000000ULL,

        // diagonal 9
        0x0000000000804020ULL,
        0x0000000080402010ULL,
        0x0000008040201008ULL,
        0x0000804020100804ULL,
        0x0080402010080402ULL,
        0x8040201008040201ULL,

        // corner
        0x0000000000000080ULL
    };
    
    for (int depth = 4; depth <= 20; depth += 2){
        uint64_t strt = tim();
        uint64_t n_solutions = 0;
        for (uint64_t condition: conditions){
            uint64_t discs = condition | 0x0000001818000000ULL;
            uint64_t put_cells = 0;
            if (depth + 4 - pop_count_ull(discs) >= 0){
                find_draw(discs, depth + 4 - pop_count_ull(discs), put_cells, &n_solutions);
            }
        }
        std::cerr << "depth " << depth << " found " << n_solutions << " solutions in " << tim() - strt << " ms" << std::endl;
    }
    return 0;
}