#include <iostream>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

#include "masks.h"

#define EPSILON 0.0000001
#define EXPLORATION sqrt(2)
#define DARK_INIT 0x0000000810000000
#define LIGHT_INIT 0x0000001008000000

/*

*/

enum class Player {
    dark = 0,
    light = 1,
};

Player opponent(Player player) {
    return static_cast<Player>(1 - static_cast<int>(player));
}

class BitBoard {
private:
    uint64_t board;
public:
    BitBoard() : board(0) {}
    BitBoard(uint64_t board) : board(board) {}
    
    void debug_print() {
        for (int i = 7; i >= 0; --i) {
            for (int j = 0; j < 8; ++j) {
                std::cout << (((board >> (8*i+j))) & 1);
            }

            std::cout << std::endl;
        }
    } 

    bool is_empty() {
        return board == 0;
    }

    int bits_set() { 
        int n = 0;
        uint64_t b = board;
        while (b != 0) {
            n += b & 1;
            b >>= 1;
        }

        return n;
    }

    int peel_bit() {
        int index = __builtin_ffsll(board) - 1;
        board &= ~((uint64_t) 1 << index);
        return index;
    }

    int peel_bit_back() {
        int index = 63 - __builtin_clzll(board);
        board &= ~((uint64_t) 1 << index);
        return index;
    }

    BitBoard operator|(BitBoard other) {
        return BitBoard(board | other.board);
    }

    BitBoard operator|(uint64_t bits) {
        return BitBoard(board | bits);
    }

    BitBoard operator&(BitBoard other) {
        return BitBoard(board & other.board);
    }

    BitBoard operator&(uint64_t bits) {
        return BitBoard(board & bits);
    }

    BitBoard operator^(BitBoard other) {
        return BitBoard(board ^ other.board);
    }

    void operator&=(BitBoard other) {
        board &= other.board;
    }

    void operator&=(uint64_t bits) {
        board &= bits;
    }

    void operator|=(BitBoard other) {
        board |= other.board;
    }

    void operator|=(uint64_t bits) {
        board |= bits;
    }

    BitBoard operator>>(int shift) {
        return BitBoard(board >> shift);
    }

    BitBoard operator<<(int shift) {
        return BitBoard(board << shift);
    }

    BitBoard operator~() {
        return BitBoard(~board);
    }

    bool operator==(BitBoard other) {
        return board == other.board;
    }

    bool operator==(uint64_t bits) {
        return bits == board;
    }
};

// hacky, but efficient
#define FILL_FUNCTION(NAME, SHIFT, MASK) \
    BitBoard NAME##_fill(BitBoard gen, BitBoard pro) { \
        BitBoard flood = gen; \
        pro &= MASK; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |=       (gen SHIFT) & pro; \
        return flood; \
    } \
    BitBoard NAME##_flood(BitBoard gen, BitBoard pro) { \
        return (NAME##_fill(gen, pro) SHIFT) & MASK; \
    } \
    BitBoard NAME##_moves(BitBoard gen, BitBoard pro) { \
        BitBoard flood = NAME##_fill(gen, pro); \
        return ((flood & pro & MASK) SHIFT); \
    }

FILL_FUNCTION(north, << 8, (uint64_t) 0xffffffffffffffff)
FILL_FUNCTION(south, >> 8, (uint64_t) 0xffffffffffffffff)
FILL_FUNCTION(west, >> 1, (uint64_t) 0x7f7f7f7f7f7f7f7f)
FILL_FUNCTION(southwest, >> 9, (uint64_t) 0x7f7f7f7f7f7f7f7f)
FILL_FUNCTION(northwest, << 7, (uint64_t) 0x7f7f7f7f7f7f7f7f)
FILL_FUNCTION(east, << 1, (uint64_t) 0xfefefefefefefefe)
FILL_FUNCTION(northeast, << 9, (uint64_t) 0xfefefefefefefefe)
FILL_FUNCTION(southeast, >> 7, (uint64_t) 0xfefefefefefefefe)
   
class Board {
public:
// private:
    BitBoard bits[2];

    BitBoard& disks(Player player) {
        return bits[static_cast<int>(player)];
    }

    BitBoard occupied() {
        return disks(Player::light) | disks(Player::light);
    }

    BitBoard move_bits(Player player) {
        BitBoard empty = ~occupied();
        BitBoard gen = disks(player);
        BitBoard pro = disks(opponent(player));
        BitBoard moves;
        moves |= north_moves(gen, pro);
        moves |= south_moves(gen, pro);
        moves |= east_moves(gen, pro);
        moves |= northeast_moves(gen, pro);
        moves |= southeast_moves(gen, pro);
        moves |= west_moves(gen, pro);
        moves |= northwest_moves(gen, pro);
        moves |= southwest_moves(gen, pro);

        return moves & empty;
    }

    Board place_disk(Player player, int index) {
        std::cout << static_cast<int>(player) << std::endl;
        Board board = *this;
        BitBoard placed = BitBoard((uint64_t) 1 << index);
        BitBoard flipping = disks(opponent(player));

        BitBoard flipped;
        // flipped |= north_flood(placed, flipping);
        // flipped |= south_flood(placed, flipping);
        // flipped |= east_flood(placed, flipping);
        // flipped |= northeast_flood(placed, flipping);
        // flipped |= southeast_flood(placed, flipping);
        // flipped |= west_flood(placed, flipping);
        // flipped |= northwest_flood(placed, flipping);
        // flipped |= southwest_flood(placed, flipping);
        // flipped = multidirectional_flood(disks(player), disks(opponent(player)), index);
        flipped.debug_print();
        
        std::cout << index << std::endl; 
        board.disks(player) |= (flipped | placed);
        board.disks(opponent(player)) &= ~(flipped);
        return board; 
    }

    Board(BitBoard dark, BitBoard light) : bits{dark, light} {}
// public:
    Board() : bits{0, 0} {}

    // Temporary
    static Board opening_position() {
        return Board(BitBoard(DARK_INIT), BitBoard(LIGHT_INIT));
    }

    void debug_print() {
        for (int i = 7; i >= 0; --i) {
            for (int j = 0; j < 8; ++j) {
                int idx = i*8 + j;   
                if (((disks(Player::dark) >> idx) & 1) == 1) {
                    std::cout << "D";
                } else if (((disks(Player::light) >> idx) & 1) == 1) {
                    std::cout << "L";
                } else {
                    std::cout << "_";
                }
            }
            
            std::cout << std::endl;    
        }
    }

    int score(Player player) {     
        return disks(player).bits_set();
    }

    bool is_winner(Player player) {
        return score(player) > score(opponent(player));
    }

    std::vector<Board> find_moves(Player player) {
        std::vector<Board> moves;

        BitBoard bits = move_bits(player);
        while (!bits.is_empty()) {
            Board board = *this;
            int index = bits.peel_bit();
            board = board.place_disk(player, index);
            moves.push_back(board);
        }

        std::cout << "Exiting find_moves" << std::endl;

        return moves;
    }
};

Player playout(Board start, Player player) {
    std::cout << "Board: " << std::endl;
    start.debug_print();

    Player turn = player;
    Board board = start;
    for (;;) {
        std::vector<Board> moves = board.find_moves(turn);
        std::cout << "Got " << moves.size() << " moves." << std::endl;
        for (Board move : moves) {
            move.debug_print();
            std::cout << std::endl;
        }

        if (moves.empty()) {
            if (board.is_winner(Player::light)) {
                return Player::light;
            } else if (board.is_winner(Player::dark)) {
                return Player::dark;
            } else {
                return static_cast<Player>(rand() % 2);
            }
        }

        std::cout << "Chosen move:" << std::endl;
        board = moves[rand() % moves.size()];
        board.debug_print();
        turn = opponent(turn);
    }
}

class Node {
private:
    Board board;
    Player player;
    int wins;
    int simulations;
    std::vector<Node> children;

    bool is_leaf() {
        return children.empty();
    }

    double utc_value() {
        double mean = wins / (simulations + EPSILON);
        return mean + EXPLORATION * sqrt(log(simulations + 1) / (simulations + EPSILON));
    }

    Node& select() {
        unsigned int max_index = -1;
        double max_value = -1;

        for (unsigned int i = 0; i < children.size(); ++i) {
            double value = children[i].utc_value();
            if (max_value < value) {
                max_value = value;
                max_index = i;
            }
        }

        return children[max_index];
    }

    void expand() {
        std::vector<Board> moves = board.find_moves(player);
        for (Board move : moves) {
            move.debug_print();
            std::cout << std::endl;
            children.push_back(Node(move, opponent(player)));
        }
    }

    Player playout() {
        return ::playout(board, player);
    }
public:
    Node(Board board, Player player) : board(board), player(player), wins(0), simulations(0), children() {}

    int mcts() {
        int win;
        if (is_leaf()) {
            expand();
            Node& node = select();
            win = node.playout() == player;
            node.wins += 1 - win;
            node.simulations += 1;
        } else {
            Node& next = select();
            win = next.mcts();
        }
 
        wins += win;
        simulations += 1;
        return win;
    }
};

int main(void) {
    srand(time(NULL));
    
    /*    
    BitBoard pro(((uint64_t) 1 << 1) | (((uint64_t) 1 << 2)) | (((uint64_t) 1 << 3)));
    BitBoard disks((uint64_t) 1 << 0);
    BitBoard move((uint64_t) 1 << 4);
    BitBoard flood = west_fill(move, pro);
    flood.debug_print();

    int i = flood.peel_bit();
    if (((disks >> (i-1)) & 1) == 1) {
        std::cout << "Did we do it?" << std::endl;
    }
    */
    
    for (int i = 0; i < sizeof(test) / sizeof(uint64_t); ++i) {
        BitBoard(test[i]).debug_print();
        std::cout << std::endl;
    }

    return 0;
}
