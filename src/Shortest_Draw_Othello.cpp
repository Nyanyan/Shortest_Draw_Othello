/*
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
#include <vector>
#include <algorithm>
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
	uint64_t s0 = (goal_mask >> 1) & (goal_mask << 1) & 0x7e7e7e7e7e7e7e7eull;
	uint64_t s1 = (goal_mask >> 8) & (goal_mask << 8);
	uint64_t s2 = (goal_mask >> 9) & (goal_mask << 9) & 0x7e7e7e7e7e7e7e7eull;
	uint64_t s3 = (goal_mask >> 7) & (goal_mask << 7) & 0x7e7e7e7e7e7e7e7eull;
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

// generate draw endgame
void generate_boards(Board *board, int n_discs_half, uint64_t *n_boards, uint64_t *n_solutions, std::vector<std::pair<Board, std::unordered_set<std::string>>> &data){
	uint64_t ba = board->player;

	uint64_t s1 = ba;
	s1 &= (s1 >> 4);
	s1 &= (s1 >> 2);
	s1 &= (s1 >> 1);
	s1 &= 0x0101010101010101ull;
	s1 *= 0xff;

	uint64_t s8 = ba;
	s8 &= (s8 >> 32) | (s8 << 32);
	s8 &= (s8 >> 16) | (s8 << 48);
	s8 &= (s8 >> 8) | (s8 << 56);

	uint64_t s7 = ~ba;
	uint64_t t7 = (s7 | (s7 >> 28)) & 0xf0f0f0f0;
	s7 |= t7; s7 |= (t7 << 28);
	t7 = (s7 | (s7 >> 14)) & 0x0000fcfcfcfcfcfcull;
	s7 |= t7; s7 |= (t7 << 14);
	t7 = (s7 | (s7 >> 7)) & 0x00fefefefefefefeull;
	s7 |= t7; s7 |= (t7 << 7);
	s7 = ~s7;

	uint64_t s9 = ~ba;
	uint64_t t9 = (s9 | (s9 >> 36)) & 0x0f0f0f0f;
	s9 |= t9; s9 |= (t9 << 36);
	t9 = (s9 | (s9 >> 18)) & 0x00003f3f3f3f3f3full;
	s9 |= t9; s9 |= (t9 << 18);
	t9 = (s9 | (s9 >> 9)) & 0x007f7f7f7f7f7f7full;
	s9 |= t9; s9 |= (t9 << 9);
	s9 = ~s9;


	std::vector<uint64_t> chunk;
	auto el = [&](uint64_t b0, uint64_t m, int d) {
		for (uint64_t b = ~((b0 << d) & m) & b0; b; b &= b - 1) {
			uint64_t x = b & -b;
			x |= (x << d) & m;
			x |= (x << d) & m;
			x |= (x << d) & m;
			x |= (x << d) & m;
			x |= (x << d) & m;
			x |= (x << d) & m;
			chunk.push_back (x);
		}
	};

	el (ba & ~s1, ba & 0xfefefefefefefefeull, 1);
	el (ba & ~s8, ba & 0xffffffffffffff00ull, 8);
	el (ba & ~s7, ba & 0x7f7f7f7f7f7f7f00ull, 7);
	el (ba & ~s9, ba & 0xfefefefefefefe00ull, 9);

	uint64_t bb = 0;
	for (auto e = chunk.begin (); e != chunk.end ();) {
		e = chunk.end ();
		for (auto i = chunk.begin (); i != chunk.end (); i++) {
			for (auto j = i + 1; j < chunk.end (); j++) {
				if (*i & *j) {
					*i |= *j;
					if (pop_count_ull (*i) > n_discs_half)
						return;
					chunk.erase (j--);
				}
			}
			bb |= *i;
		}
	}

	for (uint64_t b = ba & ~bb; b; b &= b - 1)
		chunk.push_back (b & -b);

	for (int i = 1; i < (1 << chunk.size () - 1); i++) {
		uint64_t bb = 0;
		for (int j = 0; j < chunk.size (); j++) {
			if (i & (1 << j)) {
				bb |= chunk[j];
				if (pop_count_ull (bb) > n_discs_half)
					break;
			}
		}
		if (pop_count_ull (bb) == n_discs_half) {
			board->player = bb;
			board->opponent = ba & ~bb;
    	    ++(*n_boards);
            find_path(board, n_solutions, data);
		}
	}
}

// generate silhouettes
void generate_silhouettes(uint64_t discs, int depth, uint64_t seen_cells, uint64_t *n_silhouettes, uint64_t *n_boards, uint64_t *n_solutions, bool connected, int max_memo_depth, std::vector<std::pair<Board, std::unordered_set<std::string>>> &data){
    if (!connected){
        connected = check_all_connected(discs);
    }
    if (depth == 0){
        if (connected){
            ++(*n_silhouettes);
            /*
            if (discs == (0x0000021f1c181000ULL | 0x10f840e000000402ULL)){
                std::cerr << "found" << std::endl;
                Board board;
                board.player = 0;
                board.opponent = 0;
                std::vector<uint64_t> groups = div_groups(discs, 0);
                std::cerr << groups.size() << std::endl;
                bit_print_board(discs);
                for (uint64_t elem: groups){
                    bit_print_board(elem);
                }
                generate_boards(&board, groups, 0, pop_count_ull(discs) / 2, n_boards, n_solutions);
            }
            */
            Board board;
			board.player = discs;
            board.opponent = 0;
			generate_boards(&board, pop_count_ull(discs) / 2, n_boards, n_solutions, data);
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

    // need 1 or more full line to cause gameover by draw  (because there are both black and white discs)
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
