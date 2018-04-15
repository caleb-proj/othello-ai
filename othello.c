#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

enum player {
    DARK = 0,
    LIGHT = 1,
};

struct board {
    uint64_t disks[2];
};

bool full(struct board b) {
    return (b.disks[DARK] | b.disks[LIGHT]) == ~((uint64_t) 0);
}

int score(struct board board, enum player player) {
    int n = 0;
    int d = board.disks[player];
    while (d != 0) {
        n += d & 1;
        d >>= 1;
    }

    return n;
}

bool is_winner(struct board board, enum player player) {
    return score(board, player) > score(board, 1 - player);
}

uint64_t rotate(uint64_t x, int s) {
    if (s >= 0) {
        return (x << s) | (x >> (64-s));
    } else {
        return (x >> -s) | (x << (64+s));
    }
}

// hacky, but efficient
#define FILL_FUNCTION(NAME, SHIFT, MASK) \
    uint64_t NAME##_fill(uint64_t gen, uint64_t pro) { \
        uint64_t flood = gen; \
        pro &= MASK; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |= gen = (gen SHIFT) & pro; \
        flood |=       (gen SHIFT) & pro; \
        return (flood); \
    } \
    uint64_t NAME##_moves(uint64_t gen, uint64_t pro) { \
        uint64_t flood = NAME##_fill(gen, pro); \
        return ((flood & pro) SHIFT); \
    }

FILL_FUNCTION(north, >> 8, 0xffffffffffffffff)
FILL_FUNCTION(south, << 8, 0xffffffffffffffff)
FILL_FUNCTION(west, << 1, 0xfefefefefefefefe)
FILL_FUNCTION(southwest, << 9, 0xfefefefefefefefe)
FILL_FUNCTION(northwest, >> 7, 0xfefefefefefefefe)
FILL_FUNCTION(east, >> 1, 0x7f7f7f7f7f7f7f7f)
FILL_FUNCTION(northeast, >> 9, 0x7f7f7f7f7f7f7f7f)
FILL_FUNCTION(southeast, << 7, 0x7f7f7f7f7f7f7f7f)

uint64_t move_bits(struct board board, enum player player) {
    uint64_t empty = ~(board.disks[DARK] | board.disks[LIGHT]);
    uint64_t gen = board.disks[player];
    uint64_t pro = board.disks[1 - player];
    uint64_t moves = 0;
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

int find_moves(struct board board, enum player player, uint8_t* moves) {
    uint64_t bits = move_bits(board, player);
    int n = 0;
    while (bits != 0) {
        uint8_t i = __builtin_ffsll(bits) - 1;
        moves[n++] = i;
        bits ^= 1 << i;
    }

    return n;
}

struct board place_disk(struct board board, enum player player, int i) {
    uint64_t bits = 1 << i;
    uint64_t disks = board.disks[1 - player];

    uint64_t flipped = 0;
    flipped |= north_fill(bits, disks);
    flipped |= south_fill(bits, disks);
    flipped |= east_fill(bits, disks);
    flipped |= northeast_fill(bits, disks);
    flipped |= southeast_fill(bits, disks);
    flipped |= west_fill(bits, disks);
    flipped |= northwest_fill(bits, disks);
    flipped |= southwest_fill(bits, disks);
    
    board.disks[player] |= flipped;
    board.disks[1 - player] ^= flipped;
    return board; 
}

int main(void) {
    struct board board = { .disks = { ((uint64_t) 1 << (3*8+4)) | ((uint64_t) 1 << (4*8+3)), ((uint64_t) 1 << (3*8+3)) | ((uint64_t) 1 << (4*8+4)) } };
    board = place_disk(board, DARK, 2*8+3);

    for (int i = 0; i < 64; ++i) {
        if (i % 8 == 0) {
            printf("\n");
        }

        if ((board.disks[DARK] >> i) & 1) {
            printf("D");
        } else if ((board.disks[LIGHT] >> i) & 1) {
            printf("L");
        } else {
            printf("_");
        }
    }

    printf("\n");

    return 0;
}
