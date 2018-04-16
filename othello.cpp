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

#define EAST_MASK 0xfefefefefefefefe
#define WEST_MASK 0x7f7f7f7f7f7f7f7f

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

    bool operator!=(uint64_t bits) {
        return bits != board;
    }
};

// hacky, but efficient
#define FILL_FUNCTION(NAME, SHIFT, MASK) \
    BitBoard NAME##_fill(BitBoard gen, BitBoard pro) { \
        BitBoard flood = gen; \
        pro &= MASK; \
        while (!gen.is_empty()) { \
            flood |= gen; \
            gen = (gen SHIFT) & pro; \
        } \
        return flood; \
    } \
    BitBoard NAME##_flood(BitBoard gen, BitBoard pro) { \
        return (NAME##_fill(gen, pro) SHIFT) & MASK; \
    } \
    BitBoard NAME##_moves(BitBoard gen, BitBoard pro) { \
        BitBoard flood = NAME##_fill(gen, pro); \
        return ((flood & pro) SHIFT) & MASK; \
    }

FILL_FUNCTION(north, << 8, (uint64_t) 0xffffffffffffffff)
FILL_FUNCTION(south, >> 8, (uint64_t) 0xffffffffffffffff)
FILL_FUNCTION(west, >> 1, (uint64_t) WEST_MASK)
FILL_FUNCTION(southwest, >> 9, (uint64_t) WEST_MASK)
FILL_FUNCTION(northwest, << 7, (uint64_t) WEST_MASK)
FILL_FUNCTION(east, << 1, (uint64_t) EAST_MASK)
FILL_FUNCTION(northeast, << 9, (uint64_t) EAST_MASK)
FILL_FUNCTION(southeast, >> 7, (uint64_t) EAST_MASK)
   
class Board {
// private:
public:
    BitBoard bits[2];

    BitBoard& disks(Player player) {
        return bits[static_cast<int>(player)];
    }

    BitBoard occupied() {
        return disks(Player::dark) | disks(Player::light);
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
        Board board = *this;
        BitBoard placed = BitBoard((uint64_t) 1 << index);
        BitBoard own = disks(player);
        BitBoard flipping = disks(opponent(player));

        BitBoard flipped;
    
        BitBoard gen = placed;
        BitBoard flood = gen;
        BitBoard pro = flipping;

        do {
            flood |= gen = (gen >> 8) & pro;
        } while (!gen.is_empty());
       
        if (((flood >> 8) & own) != 0) {
            flipped |= flood;
        }

        gen = placed;
        flood = gen;

        do {
            flood |= gen = (gen << 8) & pro;
        } while (!gen.is_empty());

        if (((flood << 8) & own) != 0) {
            flipped |= flood;
        }

        gen = placed;
        flood = gen;
        pro = flipping & EAST_MASK;

        do {
            flood |= gen = (gen << 1) & pro;
        } while (!gen.is_empty());
   
        if ((((flood << 1) & EAST_MASK) & own) != 0) {
            flipped |= flood;
        }

        gen = placed;
        flood = gen;
        int i = index;

        do {
            flood |= gen = (gen << 9) & pro;
            i += 9; 
        } while (!gen.is_empty());

        if (((own >> i) & 1) == 1) {
            flipped |= flood;
        }

        gen = placed;
        flood = gen;
        i = index;

        do {
            flood |= gen = (gen >> 7) & pro;
            i -= 7; 
        } while (!gen.is_empty());

        if (((own >> i) & 1) == 1) {
            flipped |= flood;
        }

        gen = placed;
        flood = gen;
        pro = flipping & WEST_MASK;
        i = index;

        do {
            flood |= gen = (gen >> 1) & pro;
        } while (!gen.is_empty());

        if ((((flood >> 1) & WEST_MASK) & own) != 0) {
            flipped |= flood;
        }

        gen = placed;
        flood = gen;
        i = index;

        do {
            flood |= gen = (gen << 7) & pro;
            i += 7; 
        } while (!gen.is_empty());

        if (((own >> i) & 1) == 1) {
            flipped |= flood;
        }

        gen = placed;
        flood = gen;
        i = index;

        do {
            flood |= gen = (gen >> 9) & pro;
            i -= 9; 
        } while (!gen.is_empty());

        if (((own >> i) & 1) == 1) {
            flipped |= flood;
        }
 
        board.disks(player) |= flipped;
        board.disks(opponent(player)) &= ~flipped;
        return board; 
    }

    Board(BitBoard dark, BitBoard light) : bits{dark, light} {}
// public:
    Board() : bits{0, 0} {}

    // Temporary
    static Board opening_position() {
        return Board(BitBoard(DARK_INIT), BitBoard(LIGHT_INIT));
    }

    void display() {
        BitBoard dark = disks(Player::dark);
        BitBoard light = disks(Player::light);

        std::cout << "\033[42m";
        std::cout << "\033[30m";

        std::cout << "┌──┬──┬──┬──┬──┬──┬──┬──┐";
        std::cout << "\033[49m";
        std::cout << std::endl;
        std::cout << "\033[42m";

        for (int i = 7; i >= 0; --i) {
            for (int j = 0; j < 8; ++j) {
                std::cout << "\033[97m";
                std::cout << "\033[30m";
                std::cout << "│";
                int idx = i*8 + j;
                if (((dark >> idx) & 1) == 1) {
                    std::cout << "\033[30m";
                    std::cout << "⬤ ";
                } else if (((light >> idx) & 1) == 1) {
                    std::cout << "\033[97m";
                    std::cout << "⬤ ";
                } else {
                    std::cout << "  ";
                }
            }
            
            std::cout << "\033[30m";
            std::cout << "│";
            std::cout << "\033[49m";
            std::cout << std::endl;
            std::cout << "\033[42m";
            if (i != 0) {  
                std::cout << "├──┼──┼──┼──┼──┼──┼──┼──┤";
                std::cout << "\033[49m";
                std::cout << std::endl;
                std::cout << "\033[42m";
            }
        }
        
        std::cout << "\033[30m"; 
        std::cout << "└──┴──┴──┴──┴──┴──┴──┴──┘";

        std::cout << "\033[39m";
        std::cout << "\033[49m";
        std::cout << std::endl;
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

        return moves;
    }
};

Player playout(Board start, Player player) {
    Player turn = player;
    Board board = start;
    for (;;) {
        std::vector<Board> moves = board.find_moves(turn);

        if (moves.empty()) {
            if (board.is_winner(Player::light)) {
                return Player::light;
            } else if (board.is_winner(Player::dark)) {
                return Player::dark;
            } else {
                return static_cast<Player>(rand() % 2);
            }
        }
            
        board = moves[rand() % moves.size()];
        turn = opponent(turn);
    }
}

class Node {
private:
    Board board;
    Player turn;
    int wins;
    int simulations;
    std::vector<Node> children;
    bool terminal_position;

    bool is_leaf() {
        return children.empty();
    }

    double utc_value(Node& node) {
        double mean = node.wins / (node.simulations + EPSILON);
        return mean + EXPLORATION * sqrt(log(simulations + 1) / (node.simulations + EPSILON));
    }

    Node& select() {
        unsigned int max_index = -1;
        double max_value = -1;

        for (unsigned int i = 0; i < children.size(); ++i) {
            double value = utc_value(children[i]);
            if (max_value < value) {
                max_value = value;
                max_index = i;
            }
        }
        
        return children[max_index];
    }

    void expand() {
        std::vector<Board> moves = board.find_moves(turn);
        if (moves.empty()) {
            terminal_position = true;
        } else {
            for (Board move : moves) {
                children.emplace_back(Node(move, opponent(turn)));
            }
        }
    }

    Player playout() {
        return ::playout(board, turn);
    }
public:
    Node(Board board, Player turn) : board(board), turn(turn), wins(0), simulations(0), children(), terminal_position(false) {}

    int mcts() {
        if (is_leaf()) {
            expand();
        }

        if (terminal_position) {
            int win;
            if (board.is_winner(Player::dark)) {
                win = Player::dark == turn;
            } else if (board.is_winner(Player::light)) {
                win = Player::light == turn;
            } else {
                int half = RAND_MAX / 2;
                if (rand() % 2 <= half)
                    win = 1;
                else
                    win = 0;
            }

            simulations += 1;
            wins += win;
            return win;
        }
        
        Node& next = select();

        int win;
        if (next.simulations == 0) {
            win = next.playout() == turn;
            next.wins += 1 - win;
            next.simulations += 1;
        } else {
            win = 1 - next.mcts();
        }

        wins += win;
        simulations += 1;
        return win;
    }

    Node& best_move() {
        unsigned int max_index = -1;
        int max_simulations = -1;
        for (unsigned int i = 0; i < children.size(); ++i) {
            std::cout << "Move " << i << ": " << children[i].wins << "/" << children[i].simulations << std::endl;

            if (children[i].simulations > max_simulations) {
                max_simulations = children[i].simulations;
                max_index = i;
            }
        }

        return children[max_index];
    }

    Board get_board() {
        return board;
    }

    bool is_terminal() {
        return terminal_position;
    }
};

int main(void) {
    srand(time(NULL));
    
    Node* iter = new Node(Board::opening_position(), Player::dark);
    while (!iter->is_terminal()) {
        iter->get_board().display();
        
        for (int i = 0; i < 15000; ++i) { 
            iter->mcts();
        }
        
        iter = &iter->best_move();
    }
    
    Board board = iter->get_board();
    if (board.is_winner(Player::dark)) {
        std::cout << "Dark wins" << std::endl;
    } else if (board.is_winner(Player::light)) {
        std::cout << "Light wins" << std::endl;
    } else {
        std::cout << "Draw" << std::endl;
    }

    return 0;
}
