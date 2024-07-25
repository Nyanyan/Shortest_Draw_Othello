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

void find_draw(Board &board, int depth, std::vector<int> &transcript){
    if (depth == 0){
        if (board.is_end() && board.score_player() == 0){
            output_transcript(transcript);
        }
        return;
    }
    if (board.is_end()){
        return;
    }
    uint64_t legal = board.get_legal();
    if (legal == 0){
        board.pass();
        legal = board.get_legal();
    }
    Flip flip;
    for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)){
        calc_flip(&flip, &board, cell);
        board.move_board(&flip);
        transcript.emplace_back(cell);
            find_draw(board, depth - 1, transcript);
        transcript.pop_back();
        board.undo_board(&flip);
    }
}

int main(int argc, char* argv[]){
    init();
    Board board;
    Flip flip;
    std::vector<int> transcript;
    board.reset();

    // play f5
    calc_flip(&flip, &board, 26);
    board.move_board(&flip);
    transcript.emplace_back(26);

    for (int depth = 1; depth <= 20; ++depth){
        uint64_t strt = tim();
        std::cout << "start depth " << depth << std::endl;
        find_draw(board, depth - 1, transcript);
        std::cout << "finish depth " << depth << " in " << tim() - strt << " ms" << std::endl;
    }
    return 0;
}