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

void enumerate_draw_boards(uint64_t discs, int depth, uint64_t put_cells, std::vector<uint64_t> &silhouettes){
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
        if (visited == discs){ // connected?
            silhouettes.emplace_back(discs);
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
                enumerate_draw_boards(discs, depth - 1, put_cells, silhouettes);
            discs ^= 1ULL << cell;
        }
    }
}

std::vector<uint64_t> lines = {
    // horizontal
    0x00000000000000FFULL,
    0x000000000000FF00ULL,
    0x0000000000FF0000ULL,
    0x00000000FF000000ULL,

    // diagonal 9
    0x0000000000008040ULL,
    0x0000000000804020ULL,
    0x0000000080402010ULL,
    0x0000008040201008ULL,
    0x0000804020100804ULL,
    0x0080402010080402ULL,
    0x8040201008040201ULL
};

int main(int argc, char* argv[]){
    init();
    
    for (int depth = 2; depth <= 20; depth += 2){
        uint64_t strt = tim();
        std::vector<uint64_t> silhouettes;
        for (uint64_t line: lines){
            uint64_t discs = line | 0x0000001818000000ULL;
            uint64_t put_cells = 0;
            if (depth + 4 - pop_count_ull(discs) >= 0){
                enumerate_draw_boards(discs, depth + 4 - pop_count_ull(discs), put_cells, silhouettes);
            }
        }
        std::cerr << "depth " << depth << " n_silhouettes " << silhouettes.size() << " in " << tim() - strt << " ms" << std::endl;
    }
    return 0;
}