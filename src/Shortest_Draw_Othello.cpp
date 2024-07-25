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
        silhouettes.emplace_back(discs);
        return;
    }
    uint64_t neighbours = 0;
    neighbours |= (discs & 0x7F7F7F7F7F7F7F7FULL) << 1;
    neighbours |= (discs & 0xFEFEFEFEFEFEFEFEULL) >> 1;
    neighbours |= (discs & 0x00FFFFFFFFFFFFFFULL) << 8;
    neighbours |= (discs & 0xFFFFFFFFFFFFFF00ULL) >> 8;
    neighbours |= (discs & 0x00FEFEFEFEFEFEFEULL) << 7;
    neighbours |= (discs & 0x7F7F7F7F7F7F7F00ULL) >> 7;
    neighbours |= (discs & 0x007F7F7F7F7F7F7FULL) << 9;
    neighbours |= (discs & 0xFEFEFEFEFEFEFE00ULL) >> 9;
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

int main(int argc, char* argv[]){
    init();
    
    for (int depth = 2; depth <= 20; depth += 2){
        uint64_t strt = tim();
        std::vector<uint64_t> silhouettes;
        uint64_t discs = 0x000000181C000000ULL; // f5
        uint64_t put_cells = 0;
        enumerate_draw_boards(discs, depth - 1, put_cells, silhouettes);
        std::cerr << "depth " << depth << " n_silhouettes " << silhouettes.size() << " in " << tim() - strt << " ms" << std::endl;
    }
    return 0;
}