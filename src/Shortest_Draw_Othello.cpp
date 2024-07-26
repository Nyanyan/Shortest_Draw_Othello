﻿/*
	Shortest Draw Othello

	@file Shortest_Draw_Othello.cpp
		Main file
	@date 2024
	@author Takuto Yamana
	@license GPL-3.0 license
*/

#include <iostream>
#include <unordered_set>
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
    
    for (int &move: transcript){
        std::cerr << idx_to_coord(move);
    }
    std::cerr << std::endl;
    
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

constexpr uint64_t bit_line[HW2][4] = {
    {0x00000000000000FEULL, 0x0101010101010100ULL, 0x0000000000000000ULL, 0x8040201008040200ULL},
    {0x00000000000000FDULL, 0x0202020202020200ULL, 0x0000000000000100ULL, 0x0080402010080400ULL},
    {0x00000000000000FBULL, 0x0404040404040400ULL, 0x0000000000010200ULL, 0x0000804020100800ULL},
    {0x00000000000000F7ULL, 0x0808080808080800ULL, 0x0000000001020400ULL, 0x0000008040201000ULL},
    {0x00000000000000EFULL, 0x1010101010101000ULL, 0x0000000102040800ULL, 0x0000000080402000ULL},
    {0x00000000000000DFULL, 0x2020202020202000ULL, 0x0000010204081000ULL, 0x0000000000804000ULL},
    {0x00000000000000BFULL, 0x4040404040404000ULL, 0x0001020408102000ULL, 0x0000000000008000ULL},
    {0x000000000000007FULL, 0x8080808080808000ULL, 0x0102040810204000ULL, 0x0000000000000000ULL},
    {0x000000000000FE00ULL, 0x0101010101010001ULL, 0x0000000000000002ULL, 0x4020100804020000ULL},
    {0x000000000000FD00ULL, 0x0202020202020002ULL, 0x0000000000010004ULL, 0x8040201008040001ULL},
    {0x000000000000FB00ULL, 0x0404040404040004ULL, 0x0000000001020008ULL, 0x0080402010080002ULL},
    {0x000000000000F700ULL, 0x0808080808080008ULL, 0x0000000102040010ULL, 0x0000804020100004ULL},
    {0x000000000000EF00ULL, 0x1010101010100010ULL, 0x0000010204080020ULL, 0x0000008040200008ULL},
    {0x000000000000DF00ULL, 0x2020202020200020ULL, 0x0001020408100040ULL, 0x0000000080400010ULL},
    {0x000000000000BF00ULL, 0x4040404040400040ULL, 0x0102040810200080ULL, 0x0000000000800020ULL},
    {0x0000000000007F00ULL, 0x8080808080800080ULL, 0x0204081020400000ULL, 0x0000000000000040ULL},
    {0x0000000000FE0000ULL, 0x0101010101000101ULL, 0x0000000000000204ULL, 0x2010080402000000ULL},
    {0x0000000000FD0000ULL, 0x0202020202000202ULL, 0x0000000001000408ULL, 0x4020100804000100ULL},
    {0x0000000000FB0000ULL, 0x0404040404000404ULL, 0x0000000102000810ULL, 0x8040201008000201ULL},
    {0x0000000000F70000ULL, 0x0808080808000808ULL, 0x0000010204001020ULL, 0x0080402010000402ULL},
    {0x0000000000EF0000ULL, 0x1010101010001010ULL, 0x0001020408002040ULL, 0x0000804020000804ULL},
    {0x0000000000DF0000ULL, 0x2020202020002020ULL, 0x0102040810004080ULL, 0x0000008040001008ULL},
    {0x0000000000BF0000ULL, 0x4040404040004040ULL, 0x0204081020008000ULL, 0x0000000080002010ULL},
    {0x00000000007F0000ULL, 0x8080808080008080ULL, 0x0408102040000000ULL, 0x0000000000004020ULL},
    {0x00000000FE000000ULL, 0x0101010100010101ULL, 0x0000000000020408ULL, 0x1008040200000000ULL},
    {0x00000000FD000000ULL, 0x0202020200020202ULL, 0x0000000100040810ULL, 0x2010080400010000ULL},
    {0x00000000FB000000ULL, 0x0404040400040404ULL, 0x0000010200081020ULL, 0x4020100800020100ULL},
    {0x00000000F7000000ULL, 0x0808080800080808ULL, 0x0001020400102040ULL, 0x8040201000040201ULL},
    {0x00000000EF000000ULL, 0x1010101000101010ULL, 0x0102040800204080ULL, 0x0080402000080402ULL},
    {0x00000000DF000000ULL, 0x2020202000202020ULL, 0x0204081000408000ULL, 0x0000804000100804ULL},
    {0x00000000BF000000ULL, 0x4040404000404040ULL, 0x0408102000800000ULL, 0x0000008000201008ULL},
    {0x000000007F000000ULL, 0x8080808000808080ULL, 0x0810204000000000ULL, 0x0000000000402010ULL},
    {0x000000FE00000000ULL, 0x0101010001010101ULL, 0x0000000002040810ULL, 0x0804020000000000ULL},
    {0x000000FD00000000ULL, 0x0202020002020202ULL, 0x0000010004081020ULL, 0x1008040001000000ULL},
    {0x000000FB00000000ULL, 0x0404040004040404ULL, 0x0001020008102040ULL, 0x2010080002010000ULL},
    {0x000000F700000000ULL, 0x0808080008080808ULL, 0x0102040010204080ULL, 0x4020100004020100ULL},
    {0x000000EF00000000ULL, 0x1010100010101010ULL, 0x0204080020408000ULL, 0x8040200008040201ULL},
    {0x000000DF00000000ULL, 0x2020200020202020ULL, 0x0408100040800000ULL, 0x0080400010080402ULL},
    {0x000000BF00000000ULL, 0x4040400040404040ULL, 0x0810200080000000ULL, 0x0000800020100804ULL},
    {0x0000007F00000000ULL, 0x8080800080808080ULL, 0x1020400000000000ULL, 0x0000000040201008ULL},
    {0x0000FE0000000000ULL, 0x0101000101010101ULL, 0x0000000204081020ULL, 0x0402000000000000ULL},
    {0x0000FD0000000000ULL, 0x0202000202020202ULL, 0x0001000408102040ULL, 0x0804000100000000ULL},
    {0x0000FB0000000000ULL, 0x0404000404040404ULL, 0x0102000810204080ULL, 0x1008000201000000ULL},
    {0x0000F70000000000ULL, 0x0808000808080808ULL, 0x0204001020408000ULL, 0x2010000402010000ULL},
    {0x0000EF0000000000ULL, 0x1010001010101010ULL, 0x0408002040800000ULL, 0x4020000804020100ULL},
    {0x0000DF0000000000ULL, 0x2020002020202020ULL, 0x0810004080000000ULL, 0x8040001008040201ULL},
    {0x0000BF0000000000ULL, 0x4040004040404040ULL, 0x1020008000000000ULL, 0x0080002010080402ULL},
    {0x00007F0000000000ULL, 0x8080008080808080ULL, 0x2040000000000000ULL, 0x0000004020100804ULL},
    {0x00FE000000000000ULL, 0x0100010101010101ULL, 0x0000020408102040ULL, 0x0200000000000000ULL},
    {0x00FD000000000000ULL, 0x0200020202020202ULL, 0x0100040810204080ULL, 0x0400010000000000ULL},
    {0x00FB000000000000ULL, 0x0400040404040404ULL, 0x0200081020408000ULL, 0x0800020100000000ULL},
    {0x00F7000000000000ULL, 0x0800080808080808ULL, 0x0400102040800000ULL, 0x1000040201000000ULL},
    {0x00EF000000000000ULL, 0x1000101010101010ULL, 0x0800204080000000ULL, 0x2000080402010000ULL},
    {0x00DF000000000000ULL, 0x2000202020202020ULL, 0x1000408000000000ULL, 0x4000100804020100ULL},
    {0x00BF000000000000ULL, 0x4000404040404040ULL, 0x2000800000000000ULL, 0x8000201008040201ULL},
    {0x007F000000000000ULL, 0x8000808080808080ULL, 0x4000000000000000ULL, 0x0000402010080402ULL},
    {0xFE00000000000000ULL, 0x0001010101010101ULL, 0x0002040810204080ULL, 0x0000000000000000ULL},
    {0xFD00000000000000ULL, 0x0002020202020202ULL, 0x0004081020408000ULL, 0x0001000000000000ULL},
    {0xFB00000000000000ULL, 0x0004040404040404ULL, 0x0008102040800000ULL, 0x0002010000000000ULL},
    {0xF700000000000000ULL, 0x0008080808080808ULL, 0x0010204080000000ULL, 0x0004020100000000ULL},
    {0xEF00000000000000ULL, 0x0010101010101010ULL, 0x0020408000000000ULL, 0x0008040201000000ULL},
    {0xDF00000000000000ULL, 0x0020202020202020ULL, 0x0040800000000000ULL, 0x0010080402010000ULL},
    {0xBF00000000000000ULL, 0x0040404040404040ULL, 0x0080000000000000ULL, 0x0020100804020100ULL},
    {0x7F00000000000000ULL, 0x0080808080808080ULL, 0x0000000000000000ULL, 0x0040201008040201ULL}
};

constexpr uint64_t bit_neighbour[HW2] = {
    0x0000000000000302ULL, 0x0000000000000705ULL, 0x0000000000000E0AULL, 0x0000000000001C14ULL, 0x0000000000003828ULL, 0x0000000000007050ULL, 0x000000000000E0A0ULL, 0x000000000000C040ULL, 
    0x0000000000030203ULL, 0x0000000000070507ULL, 0x00000000000E0A0EULL, 0x00000000001C141CULL, 0x0000000000382838ULL, 0x0000000000705070ULL, 0x0000000000E0A0E0ULL, 0x0000000000C040C0ULL, 
    0x0000000003020300ULL, 0x0000000007050700ULL, 0x000000000E0A0E00ULL, 0x000000001C141C00ULL, 0x0000000038283800ULL, 0x0000000070507000ULL, 0x00000000E0A0E000ULL, 0x00000000C040C000ULL, 
    0x0000000302030000ULL, 0x0000000705070000ULL, 0x0000000E0A0E0000ULL, 0x0000001C141C0000ULL, 0x0000003828380000ULL, 0x0000007050700000ULL, 0x000000E0A0E00000ULL, 0x000000C040C00000ULL, 
    0x0000030203000000ULL, 0x0000070507000000ULL, 0x00000E0A0E000000ULL, 0x00001C141C000000ULL, 0x0000382838000000ULL, 0x0000705070000000ULL, 0x0000E0A0E0000000ULL, 0x0000C040C0000000ULL, 
    0x0003020300000000ULL, 0x0007050700000000ULL, 0x000E0A0E00000000ULL, 0x001C141C00000000ULL, 0x0038283800000000ULL, 0x0070507000000000ULL, 0x00E0A0E000000000ULL, 0x00C040C000000000ULL, 
    0x0302030000000000ULL, 0x0705070000000000ULL, 0x0E0A0E0000000000ULL, 0x1C141C0000000000ULL, 0x3828380000000000ULL, 0x7050700000000000ULL, 0xE0A0E00000000000ULL, 0xC040C00000000000ULL, 
    0x0203000000000000ULL, 0x0507000000000000ULL, 0x0A0E000000000000ULL, 0x141C000000000000ULL, 0x2838000000000000ULL, 0x5070000000000000ULL, 0xA0E0000000000000ULL, 0x40C0000000000000ULL
};


// generate draw endgame
void generate_boards(Board *board, uint64_t any_color_discs, uint64_t fixed_color_discs, int n_discs_half, uint64_t *n_boards, uint64_t *n_solutions){
    if (any_color_discs == 0 && fixed_color_discs == 0){
        if (board->is_end()){ // game over
            ++(*n_boards);
            find_path(board, n_solutions);
        }
        return;
    }
    int n_player = pop_count_ull(board->player);
    int n_opponent = pop_count_ull(board->opponent);
    if (any_color_discs){
        uint_fast8_t cell = ctz(any_color_discs);
        uint64_t cell_bit = 1ULL << cell;
        any_color_discs ^= cell_bit;
        if (n_player < n_discs_half){
            board->player ^= cell_bit;
                generate_boards(board, any_color_discs, fixed_color_discs, n_discs_half, n_boards, n_solutions);
            board->player ^= cell_bit;
        }
        if (n_opponent < n_discs_half){
            board->opponent ^= cell_bit;
                generate_boards(board, any_color_discs, fixed_color_discs, n_discs_half, n_boards, n_solutions);
            board->opponent ^= cell_bit;
        }
    } else{
        uint64_t tmp_fcd = fixed_color_discs;
        uint_fast8_t cell = ctz(tmp_fcd);
        uint64_t cell_bit = 1ULL << cell;
        while (((board->opponent & bit_neighbour[cell]) == 0 && (board->player & bit_neighbour[cell]) == 0)){
            tmp_fcd ^= cell_bit;
            cell = ctz(tmp_fcd);
            cell_bit = 1ULL << cell;
        }
        fixed_color_discs ^= cell_bit;
        if ((board->player & bit_neighbour[cell]) && (board->opponent & bit_neighbour[cell]) == 0 && n_player < n_discs_half){
            board->player ^= cell_bit;
                generate_boards(board, any_color_discs, fixed_color_discs, n_discs_half, n_boards, n_solutions);
            board->player ^= cell_bit;
        } else if ((board->opponent & bit_neighbour[cell]) && (board->player & bit_neighbour[cell]) == 0 && n_opponent < n_discs_half){
            board->opponent ^= cell_bit;
                generate_boards(board, any_color_discs, fixed_color_discs, n_discs_half, n_boards, n_solutions);
            board->opponent ^= cell_bit;
        } else if ((board->opponent & bit_neighbour[cell]) == 0 && (board->player & bit_neighbour[cell]) == 0){
            std::cerr << "ERR" << std::endl;
            bit_print_board(cell_bit);
            bit_print_board(fixed_color_discs);
            board->print();
            exit(0);
        }
    }
}

uint64_t get_neighbour_discs(uint64_t discs){
    uint64_t neighbours = 0;
    neighbours |= ((discs & 0x7F7F7F7F7F7F7F7FULL) << 1);
    neighbours |= ((discs & 0xFEFEFEFEFEFEFEFEULL) >> 1);
    neighbours |= ((discs & 0x00FFFFFFFFFFFFFFULL) << 8);
    neighbours |= ((discs & 0xFFFFFFFFFFFFFF00ULL) >> 8);
    neighbours |= ((discs & 0x00FEFEFEFEFEFEFEULL) << 7);
    neighbours |= ((discs & 0x7F7F7F7F7F7F7F00ULL) >> 7);
    neighbours |= ((discs & 0x007F7F7F7F7F7F7FULL) << 9);
    neighbours |= ((discs & 0xFEFEFEFEFEFEFE00ULL) >> 9);
    neighbours &= ~discs;
    return neighbours;
}

uint64_t get_unique_discs(uint64_t discs){
    uint64_t res = discs;
    res = std::min(res, vertical_mirror(discs));
    res = std::min(res, horizontal_mirror(discs));
    res = std::min(res, black_line_mirror(discs));
    res = std::min(res, white_line_mirror(discs));
    res = std::min(res, rotate_90(discs));
    res = std::min(res, rotate_180(discs));
    res = std::min(res, rotate_270(discs));
    return res;
}

// check whether all discs are connected
bool check_all_connected(uint64_t discs){
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
    return visited == discs;
}

// generate silhouettes
void generate_silhouettes(uint64_t discs, int depth, uint64_t seen_cells, uint64_t *n_silhouettes, uint64_t *n_boards, uint64_t *n_solutions, std::unordered_set<uint64_t> &seen_unique_discs, bool connected){
    uint64_t unique_discs = get_unique_discs(discs);
    if (seen_unique_discs.find(unique_discs) != seen_unique_discs.end()){
        return;
    }
    seen_unique_discs.emplace(unique_discs);
    if (depth == 0){
        if (!connected){
            connected = check_all_connected(discs);
        }
        if (connected){
            ++(*n_silhouettes);
            uint64_t any_color_discs = 0;
            uint64_t discs_copy = discs;
            for (uint_fast8_t cell = first_bit(&discs_copy); discs_copy; cell = next_bit(&discs_copy)){
                for (int i = 0; i < 4; ++i){
                    if ((discs & bit_line[cell][i]) == bit_line[cell][i]){
                        any_color_discs |= 1ULL << cell;
                    }
                }
            }
            uint64_t fixed_color_discs = discs & ~any_color_discs;
            if (any_color_discs){
                Board board;
                board.player = 1ULL << ctz(any_color_discs);
                board.opponent = 0;
                any_color_discs &= ~board.player;
                generate_boards(&board, any_color_discs, fixed_color_discs, pop_count_ull(discs) / 2, n_boards, n_solutions);
            }
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
    uint64_t puttable = neighbours & ~seen_cells;
    /*
    if (black_line_mirror(discs) == discs){
        puttable &= 0xFFFEFCF8F0E0C080ULL;
    }
    if (white_line_mirror(discs) == discs){
        puttable &= 0xFF7F3F1F0F070301ULL;
    }
    if (horizontal_mirror(discs) == discs){
        puttable &= 0x0F0F0F0F0F0F0F0FULL;
    }
    if (vertical_mirror(discs) == discs){
        puttable &= 0xFFFFFFFF00000000ULL;
    }
    */
    if (puttable){
        for (uint_fast8_t cell = first_bit(&puttable); puttable; cell = next_bit(&puttable)){
            uint64_t cell_bit = 1ULL << cell;
            seen_cells ^= cell_bit;
            discs ^= cell_bit;
                generate_silhouettes(discs, depth - 1, seen_cells, n_silhouettes, n_boards, n_solutions, seen_unique_discs, connected);
            discs ^= cell_bit;
        }
    }
}

int main(int argc, char* argv[]){
    init();

    // need 1 or more full line to cause gameover by draw  (because there are both black and white discs)
    std::vector<uint64_t> conditions = {
        // connected
        // horizontal
        //0x0000000000FF0000ULL // a6-h6 done
        //0x00000000FF000000ULL // a5-h5 done
        // diagonal 9
        //0x0000008040201008ULL, // a4-e8
        0x0000804020100804ULL // a3-f8
        //0x0080402010080402ULL // a2-g8 done
        //0x8040201008040201ULL // a1-h8 done


        // not connected
        // horizontal
        //0x00000000000000FFULL, // a8-h8
        //0x000000000000FF00ULL, // a7-h7
        // diagonal 9
        //0x0000000000804020ULL, // a6-c8
        //0x0000000080402010ULL, // a5-d8
        // corner
        //0x0000000000000001ULL // h8
    };
    for (uint64_t condition: conditions){
        for (uint32_t i = 0; i < HW2; ++i){
            std::cerr << (1 & (condition >> (HW2_M1 - i)));
            if (i % HW == HW_M1)
                std::cerr << std::endl;
        }
        std::cerr << std::endl;
        for (uint32_t i = 0; i < HW2; ++i){
            std::cout << (1 & (condition >> (HW2_M1 - i)));
            if (i % HW == HW_M1)
                std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    for (int depth = 4; depth <= 20; depth += 2){
        uint64_t strt = tim();
        uint64_t n_silhouettes = 0, n_boards = 0, n_solutions = 0;
        std::unordered_set<uint64_t> seen_unique_discs;
        for (uint64_t condition: conditions){
            uint64_t discs = condition | 0x0000001818000000ULL;
            uint64_t seen_cells = 0;
            if (depth + 4 - pop_count_ull(discs) >= 0){
                generate_silhouettes(discs, depth + 4 - pop_count_ull(discs), seen_cells, &n_silhouettes, &n_boards, &n_solutions, seen_unique_discs, check_all_connected(discs));
            }
        }
        std::cout << "depth " << depth << " n_silhouettes " << n_silhouettes << " n_boards " << n_boards << " n_solutions " << n_solutions << " time " << tim() - strt << " ms" << std::endl;
        std::cerr << "depth " << depth << " n_silhouettes " << n_silhouettes << " n_boards " << n_boards << " n_solutions " << n_solutions << " time " << tim() - strt << " ms" << std::endl;
    }
    return 0;
}