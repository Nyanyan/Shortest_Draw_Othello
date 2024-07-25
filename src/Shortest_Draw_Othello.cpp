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

// generate draw endgame
// nCr(24, 12) = 2.7e6 <= small enough to generate all boards and check game over
void generate_check_boards(uint64_t discs, uint64_t *n_solutions){
    Board board;
    std::vector<uint_fast8_t> cells;
    uint64_t discs_copy = discs;
    for (uint_fast8_t cell = first_bit(&discs_copy); discs_copy; cell = next_bit(&discs_copy)){
        cells.emplace_back(cell);
    }
    int n_discs = pop_count_ull(discs);
    uint64_t max_bits = 1 << n_discs;
    for (uint64_t bits = 0; bits < max_bits; ++bits){
        if (pop_count_ull(bits) == n_discs / 2){ // draw
            board.player = 0;
            board.opponent = 0;
            for (int i = 0; i < n_discs; ++i){
                if (1 & (bits >> i)){
                    board.player |= 1ULL << cells[i];
                } else{
                    board.opponent |= 1ULL << cells[i];
                }
            }
            if (board.is_end()){ // game over
                board.print();
                ++(*n_solutions);
            }
        }
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
            generate_check_boards(discs, n_solutions);
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
    
    for (int depth = 2; depth <= 20; depth += 2){
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