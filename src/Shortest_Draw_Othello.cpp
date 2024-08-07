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
#include <fstream>
#include "engine/board.hpp"
#include "engine/util.hpp"

void init(){
    bit_init();
    mobility_init();
    flip_init();
}

void output_transcript(std::vector<int> &transcript){
    std::cout << "SOLUTION ";
    for (int &move: transcript){
        std::cout << idx_to_coord(move);
    }
    std::cout << std::endl;
    
    for (int &move: transcript){
        std::cerr << idx_to_coord(move);
    }
    std::cerr << std::endl;
}

std::string generate_transcript(std::vector<int> &transcript){
    std::string res;
    for (int &move: transcript){
        res += idx_to_coord(move);
    }
    return res;
}

std::string get_unique_transcript(std::string transcript){
    std::string first_move = transcript.substr(0, 2);
    if (first_move == "f5"){
        return transcript;
    }
    std::string res;
    if (first_move == "e6"){
        for (int i = 0; i < transcript.size(); i += 2){
            int x = transcript[i] - 'a';
            int y = transcript[i + 1] - '1';
            res += (char)('a' + y);
            res += (char)('1' + x);
        }
    } else if (first_move == "d3"){
        for (int i = 0; i < transcript.size(); i += 2){
            int x = transcript[i] - 'a';
            int y = transcript[i + 1] - '1';
            res += (char)('a' + 7 - y);
            res += (char)('1' + 7 - x);
        }
    } else if (first_move == "c4"){
        for (int i = 0; i < transcript.size(); i += 2){
            int x = transcript[i] - 'a';
            int y = transcript[i + 1] - '1';
            res += (char)('a' + 7 - x);
            res += (char)('1' + 7 - y);
        }
    }
    return res;
}

std::string get_board_str(Board board){
    std::string res;
    for (int i = HW2_M1; i >= 0; --i){
        if (1 & (board.player >> i))
            res += "X";
        else if (1 & (board.opponent >> i))
            res += "O";
        else
            res += "-";
    }
    return res;
}

inline void first_update_unique_board(Board *res, Board *sym){
    uint64_t vp = vertical_mirror(sym->player);
    uint64_t vo = vertical_mirror(sym->opponent);
    if (res->player > vp || (res->player == vp && res->opponent > vo)){
        res->player = vp;
        res->opponent = vo;
    }
}

inline void update_unique_board(Board *res, Board *sym){
    if (res->player > sym->player || (res->player == sym->player && res->opponent > sym->opponent))
        sym->copy(res);
    uint64_t vp = vertical_mirror(sym->player);
    uint64_t vo = vertical_mirror(sym->opponent);
    if (res->player > vp || (res->player == vp && res->opponent > vo)){
        res->player = vp;
        res->opponent = vo;
    }
}

inline Board get_unique_board(Board b){
    Board res = b;
    first_update_unique_board(&res, &b);
    b.board_black_line_mirror();
    update_unique_board(&res, &b);
    b.board_horizontal_mirror();
    update_unique_board(&res, &b);
    b.board_white_line_mirror();
    update_unique_board(&res, &b);
    return res;
}

Board play_board(std::string transcript){
    Board board;
    board.reset();
    Flip flip;
    for (int i = 0; i < transcript.size(); i += 2){
        int x = transcript[i] - 'a';
        int y = transcript[i + 1] - '1';
        int coord = y * HW + x;
        calc_flip(&flip, &board, coord);
        board.move_board(&flip);
    }
    return board;
}

/***** from https://github.com/Nyanyan/Reverse_Othello start *****/
inline uint64_t fULL_stability_h(uint64_t fULL){
    fULL &= fULL >> 1;
    fULL &= fULL >> 2;
    fULL &= fULL >> 4;
    return (fULL & 0x0101010101010101ULL) * 0xFF;
}

inline uint64_t fULL_stability_v(uint64_t fULL){
    fULL &= (fULL >> 8) | (fULL << 56);
    fULL &= (fULL >> 16) | (fULL << 48);
    fULL &= (fULL >> 32) | (fULL << 32);
    return fULL;
}

inline void fULL_stability_d(uint64_t fULL, uint64_t *fULL_d7, uint64_t *fULL_d9){
    constexpr uint64_t edge = 0xFF818181818181FFULL;
    uint64_t l7, r7, l9, r9;
    l7 = r7 = fULL;
    l7 &= edge | (l7 >> 7);        r7 &= edge | (r7 << 7);
    l7 &= 0xFFFF030303030303ULL | (l7 >> 14);    r7 &= 0xC0C0C0C0C0C0FFFFULL | (r7 << 14);
    l7 &= 0xFFFFFFFF0F0F0F0FULL | (l7 >> 28);    r7 &= 0xF0F0F0F0FFFFFFFFULL | (r7 << 28);
    *fULL_d7 = l7 & r7;

    l9 = r9 = fULL;
    l9 &= edge | (l9 >> 9);        r9 &= edge | (r9 << 9);
    l9 &= 0xFFFFC0C0C0C0C0C0ULL | (l9 >> 18);    r9 &= 0x030303030303FFFFULL | (r9 << 18);
    *fULL_d9 = l9 & r9 & (0x0F0F0F0FF0F0F0F0ULL | (l9 >> 36) | (r9 << 36));
}

inline void fULL_stability(uint64_t discs, uint64_t *h, uint64_t *v, uint64_t *d7, uint64_t *d9){
    *h = fULL_stability_h(discs);
    *v = fULL_stability_v(discs);
    fULL_stability_d(discs, d7, d9);
}

inline uint64_t enhanced_stability(Board *board, const uint64_t goal_mask){
    uint64_t fULL_h, fULL_v, fULL_d7, fULL_d9;
    uint64_t discs = board->player | board->opponent;
    fULL_stability(discs | ~goal_mask, &fULL_h, &fULL_v, &fULL_d7, &fULL_d9);
    fULL_h &= goal_mask;
    fULL_v &= goal_mask;
    fULL_d7 &= goal_mask;
    fULL_d9 &= goal_mask;
    uint64_t h, v, d7, d9;
    uint64_t stability = 0ULL, n_stability;
    n_stability = discs & (fULL_h & fULL_v & fULL_d7 & fULL_d9);
    while (n_stability & ~stability){
        stability |= n_stability;
        h = (stability >> 1) | (stability << 1) | fULL_h;
        v = (stability >> 8) | (stability << 8) | fULL_v;
        d7 = (stability >> 7) | (stability << 7) | fULL_d7;
        d9 = (stability >> 9) | (stability << 9) | fULL_d9;
        n_stability = h & v & d7 & d9;
    }
    return stability;
}

void find_path_p(Board *board, std::vector<int> &path, int player, const uint64_t goal_mask, const uint64_t corner_mask, const int goal_n_discs, const Board *goal, const int goal_player, uint64_t *n_nodes, uint64_t *n_solutions, std::vector<std::pair<Board, std::unordered_set<std::string>>> &data){
    ++(*n_nodes);
    if (
        (player == goal_player && board->player == goal->player && board->opponent == goal->opponent) || 
        (player != goal_player && board->player == goal->opponent && board->opponent == goal->player)
        ){
        //output_transcript(path);
        std::string unique_transcript = get_unique_transcript(generate_transcript(path));
        Board unique_board = get_unique_board(board->copy());
        bool pushed = false;
        for (std::pair<Board, std::unordered_set<std::string>> &datum: data){
            if (datum.first == unique_board){
                datum.second.emplace(unique_transcript);
                pushed = true;
                break;
            }
        }
        if (!pushed){
            std::unordered_set<std::string> set;
            set.emplace(unique_transcript);
            data.emplace_back(std::make_pair(unique_board, set));
        }
        std::cout << "SOLUTION " << unique_transcript << std::endl;
        std::cerr << "SOLUTION " << unique_transcript << std::endl;
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
                find_path_p(board, path, player ^ 1, goal_mask, corner_mask, goal_n_discs, goal, goal_player, n_nodes, n_solutions, data);
            path.pop_back();
            board->undo_board(&flip);
        }
    }
}

// find path to the given board
void find_path(Board *goal, uint64_t *n_solutions, std::vector<std::pair<Board, std::unordered_set<std::string>>> &data){
    uint64_t goal_mask = goal->player | goal->opponent; // legal candidate
    uint64_t s0 = (goal_mask >> 1) & (goal_mask << 1) & 0x7e7e7e7e7e7e7e7eULL;
    uint64_t s1 = (goal_mask >> 8) & (goal_mask << 8);
    uint64_t s2 = (goal_mask >> 9) & (goal_mask << 9) & 0x7e7e7e7e7e7e7e7eULL;
    uint64_t s3 = (goal_mask >> 7) & (goal_mask << 7) & 0x7e7e7e7e7e7e7e7eULL;
    uint64_t corner_mask = goal_mask & ~(s0 | s1 | s2 | s3);
    
    //bit_print_board(goal_mask);
    //bit_print_board(corner_mask);

    int n_discs = pop_count_ull(goal_mask);
    Board board = {0x0000000810000000ULL, 0x0000001008000000ULL};
    std::vector<int> path;
    uint64_t strt = tim();
    uint64_t n_nodes = 0;
    find_path_p(&board, path, BLACK, goal_mask, corner_mask, n_discs, goal, BLACK, &n_nodes, n_solutions, data);
    find_path_p(&board, path, BLACK, goal_mask, corner_mask, n_discs, goal, WHITE, &n_nodes, n_solutions, data);
    Board goal_mirrored = goal->copy();
    goal_mirrored.board_vertical_mirror();
    find_path_p(&board, path, BLACK, goal_mask, corner_mask, n_discs, &goal_mirrored, BLACK, &n_nodes, n_solutions, data);
    find_path_p(&board, path, BLACK, goal_mask, corner_mask, n_discs, &goal_mirrored, WHITE, &n_nodes, n_solutions, data);
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

// generate draw endgame
void generate_boards(uint64_t silhouette, int n_discs_half, uint64_t *n_boards, uint64_t *n_solutions, std::vector<std::pair<Board, std::unordered_set<std::string>>> &data){
    std::vector<uint64_t> chunk;
    uint64_t silhouette_cpy = silhouette;
    for (uint_fast8_t cell = first_bit(&silhouette_cpy); silhouette_cpy; cell = next_bit(&silhouette_cpy)) {
        uint64_t cell_bit = 1ULL << cell;
        uint64_t group = cell_bit;
        if (bit_line[cell][0] & ~silhouette) {
            group |= (cell_bit << 1) & 0xFEFEFEFEFEFEFEFEULL;
        }
        if (bit_line[cell][1] & ~silhouette) {
            group |= (cell_bit << 8);
        }
        if (bit_line[cell][2] & ~silhouette) {
            group |= (cell_bit << 7) & 0x7F7F7F7F7F7F7F00ULL;
        }
        if (bit_line[cell][3] & ~silhouette) {
            group |= (cell_bit << 9) & 0xFEFEFEFEFEFEFE00ULL;
        }
        chunk.emplace_back(group & silhouette);
    }

    uint64_t bb = 0;
    for (auto e = chunk.begin(); e != chunk.end();) {
        e = chunk.end();
        for (auto i = chunk.begin(); i != chunk.end(); ++i) {
            for (auto j = i + 1; j < chunk.end(); ++j) {
                if (*i & *j) {
                    *i |= *j;
                    if (pop_count_ull(*i) > n_discs_half) {
                        return;
                    }
                    chunk.erase(j--);
                }
            }
            bb |= *i;
        }
    }

    Board board;
    for (int i = 1; i < (1 << chunk.size() - 1); ++i) {
        uint64_t player = 0;
        for (int j = 0; j < chunk.size(); ++j) {
            if (i & (1 << j)) {
                player |= chunk[j];
                if (pop_count_ull(player) > n_discs_half) {
                    break;
                }
            }
        }
        if (pop_count_ull(player) == n_discs_half) {
            board.player = player;
            board.opponent = silhouette & ~player;
            ++(*n_boards);
            find_path(&board, n_solutions, data);
        }
    }
}

// check whether all discs are connected
bool check_all_connected(uint64_t discs){
    uint64_t visited = 0;
    uint64_t n_visited = 1ULL << ctz(discs);
    while (visited != n_visited){
        visited = n_visited;
        n_visited |= (visited & 0x7F7F7F7F7F7F7F7FULL) << 1;
        n_visited |= (visited & 0xFEFEFEFEFEFEFEFEULL) >> 1;
        n_visited |= visited << 8;
        n_visited |= visited >> 8;
        n_visited |= (visited & 0x00FEFEFEFEFEFEFEULL) << 7;
        n_visited |= (visited & 0x7F7F7F7F7F7F7F00ULL) >> 7;
        n_visited |= (visited & 0x007F7F7F7F7F7F7FULL) << 9;
        n_visited |= (visited & 0xFEFEFEFEFEFEFE00ULL) >> 9;
        n_visited &= discs;
    }
    return visited == discs;
}

// generate silhouettes
void generate_silhouettes(uint64_t discs, int depth, uint64_t seen_cells, uint64_t *n_silhouettes, uint64_t *n_boards, uint64_t *n_solutions, bool connected, int max_memo_depth, std::vector<std::pair<Board, std::unordered_set<std::string>>> &data){
    if (!connected){
        connected = check_all_connected(discs);
    }
    if (depth == 0){
        if (connected){
            ++(*n_silhouettes);
            generate_boards(discs, pop_count_ull(discs) / 2, n_boards, n_solutions, data);
        }
        return;
    }
    uint64_t neighbours = 0;
    neighbours |= ((discs & 0x7F7F7F7F7F7F7F7FULL) << 1) & ((discs & 0x3F3F3F3F3F3F3F3FULL) << 2);
    neighbours |= ((discs & 0xFEFEFEFEFEFEFEFEULL) >> 1) & ((discs & 0xFCFCFCFCFCFCFCFCULL) >> 2);
    neighbours |= (discs << 8) & (discs << 16);
    neighbours |= (discs >> 8) & (discs >> 16);
    neighbours |= ((discs & 0x00FEFEFEFEFEFEFEULL) << 7) & ((discs & 0x0000FCFCFCFCFCFCULL) << 14);
    neighbours |= ((discs & 0x7F7F7F7F7F7F7F00ULL) >> 7) & ((discs & 0x3F3F3F3F3F3F0000ULL) >> 14);
    neighbours |= ((discs & 0x007F7F7F7F7F7F7FULL) << 9) & ((discs & 0x00003F3F3F3F3F3FULL) << 18);
    neighbours |= ((discs & 0xFEFEFEFEFEFEFE00ULL) >> 9) & ((discs & 0xFCFCFCFCFCFC0000ULL) >> 18);
    neighbours &= ~discs;
    uint64_t puttable = neighbours & ~seen_cells;
    /*
    // to consider mirroring (slow)
    if (black_line_mirror(discs) == discs){
        puttable ^= black_line_mirror(puttable) & puttable & 0xFFFEFCF8F0E0C080ULL;
    }
    if (white_line_mirror(discs) == discs){
        puttable ^= white_line_mirror(puttable) & puttable & 0xFF7F3F1F0F070301ULL;
    }
    if (rotate_180(discs) == discs){
        puttable ^= rotate_180(puttable) & puttable & 0xFFFFFFFF00000000ULL;
    }
    if (horizontal_mirror(discs) == discs){
        puttable ^= horizontal_mirror(puttable) & puttable & 0x0F0F0F0F0F0F0F0FULL;
    }
    */
    if (puttable){
        for (uint_fast8_t cell = first_bit(&puttable); puttable; cell = next_bit(&puttable)){
            uint64_t cell_bit = 1ULL << cell;
            seen_cells ^= cell_bit;
            discs ^= cell_bit;
                generate_silhouettes(discs, depth - 1, seen_cells, n_silhouettes, n_boards, n_solutions, connected, max_memo_depth, data);
            discs ^= cell_bit;
        }
    }
}

struct Task{
    uint64_t first_silhouette;
    int max_memo_depth;
};

int main(int argc, char* argv[]){
    init();

    // need 1 or more fULL line to cause gameover by draw  (because there are both black and white discs)
    const std::vector<Task> tasks = {
        {0x8040201008040201ULL | 0x0000001818000000ULL, 20}, // a1-h8
        {0x0000000000FF0000ULL | 0x0000001818000000ULL, 20}, // a6-h6
        {0x00000000FF000000ULL | 0x0000001818000000ULL, 20}, // a5-h5
        {0x0080402010080402ULL | 0x0000001818000000ULL, 20}, // a2-g8
        {0x0000804020100804ULL | 0x0000001818000000ULL, 20}, // a3-f8
        {0x0000008040201008ULL | 0x0000001818000000ULL, 20}, // a4-e8
        {0x000000000000FF00ULL | 0x0000001818000000ULL, 20}, // a7-h7
        {0x00000000000000FFULL | 0x0000001818000000ULL, 20}, // a8-h8
        {0x0000000080402010ULL | 0x0000001818000000ULL, 18}, // a5-d8
        {0x0000000000804020ULL | 0x0000001818000000ULL, 18}, // a6-c8
        {0x0000000020408070ULL | 0x0000001818000000ULL, 20}, // a7-b8, b6-c5, c8-d8
        {0x0000000080808070ULL | 0x0000001818000000ULL, 20}  // a7-b8, a5-a6, c8-d8
    };
    std::cout << "task list:" << std::endl;
    for (Task task: tasks){
        std::cout << "max memo depth " << task.max_memo_depth << std::endl;
        for (uint32_t i = 0; i < HW2; ++i){
            std::cout << (1 & (task.first_silhouette >> (HW2_M1 - i)));
            if (i % HW == HW_M1)
                std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    uint64_t strt = tim();
    for (int depth = 2; depth <= 20; depth += 2){
        std::vector<std::pair<Board, std::unordered_set<std::string>>> data;
        std::cout << "depth " << depth << " start" << std::endl;
        std::cerr << "depth " << depth << " start" << std::endl;
        uint64_t sum_n_silhouettes = 0, sum_n_boards = 0, sum_n_solutions = 0;
        int task_idx = 0;
        for (Task task: tasks){
            std::cerr << "\rtask " << task_idx << "/" << tasks.size();
            std::cout << "task " << task_idx << "/" << tasks.size() << std::endl;
            uint64_t n_silhouettes = 0, n_boards = 0, n_solutions = 0;
            if (depth + 4 - pop_count_ull(task.first_silhouette) >= 0){
                generate_silhouettes(task.first_silhouette, depth + 4 - pop_count_ull(task.first_silhouette), 0, &n_silhouettes, &n_boards, &n_solutions, false, task.max_memo_depth, data);
            }
            sum_n_silhouettes += n_silhouettes;
            sum_n_boards += n_boards;
            sum_n_solutions += n_solutions;
            ++task_idx;
        }
        std::cerr << std::endl;
        int n_transcript = 0;
        for (std::pair<Board, std::unordered_set<std::string>> &datum: data){
            n_transcript += datum.second.size();
        }
        std::cout << "depth " << depth << " unique transcript: " << n_transcript << " unique board: " << data.size() << " n_silhouettes " << sum_n_silhouettes << " n_boards " << sum_n_boards << " n_solutions " << sum_n_solutions << " time " << tim() - strt << " ms" << std::endl;
        std::cerr << "depth " << depth << " unique transcript: " << n_transcript << " unique board: " << data.size() << " n_silhouettes " << sum_n_silhouettes << " n_boards " << sum_n_boards << " n_solutions " << sum_n_solutions << " time " << tim() - strt << " ms" << std::endl;

        std::ofstream ofs("summarized.txt", std::ios::app);
        ofs << "depth " << depth << " unique transcript: " << n_transcript << " unique board: " << data.size() << std::endl;
        for (std::pair<Board, std::unordered_set<std::string>> &datum: data){
            Board board = play_board(*datum.second.begin());
            ofs << get_board_str(board) << std::endl;
            for (std::string t: datum.second){
                ofs << t << std::endl;
            }
        }
    }
    
    return 0;
}
