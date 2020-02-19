#include <iostream>
#include <cassert>
#include <cstdlib>
using namespace std;

#define popcount __builtin_popcount 

typedef __uint8_t uint8;
typedef __uint32_t uint32;
typedef uint32 node;       // 0..5
typedef uint32 edge;       // 0..14
typedef uint32 edge_mask;  // 15bit
typedef uint32 board;      // 30bit
const int BOARD_COUNT = (1 << 30);
const int EDGE_MASK = (1 << 15) - 1;
uint8 red_wins_table[BOARD_COUNT >> 3];
uint8 blue_wins_table[BOARD_COUNT >> 3];

edge node2edge[6][6];
node edge2node_i[15];
node edge2node_j[15];
static void init_table ()
{
    edge node_i2edge[6] = { 0, 5, 9, 12, 14, 15 };

    for (int i = 0; i < 6; i++) {
        for (int j = i + 1; j < 6; j++) {
            edge e = node_i2edge[i] + (j - i - 1);
            assert (e < 15);
            node2edge[i][j] = e;
            node2edge[j][i] = e;
            edge2node_i[e] = i;
            edge2node_j[e] = j;
            //cout << i << ',' << j << ',' << e << endl;
        }
    }
}


static inline bool red_wins(board b)
{
    return (red_wins_table[b >> 3] & (1 << (b&7))) != 0;
}

static inline bool blue_wins(board b)
{
    return (blue_wins_table[b >> 3] & (1 << (b&7))) != 0;
}

static inline bool wins(board b, bool red)
{
    if (red) return red_wins(b);
    else return blue_wins(b);
}

static inline void set_wins(board b, bool red)
{
    if (red) {
        red_wins_table[b >> 3] |= (1 << (b&7));
    } else {
        blue_wins_table[b >> 3] |= (1 << (b&7));
    }
}

static inline edge_mask board2edge_mask(board b, bool red)
{
    edge_mask emask;
    if (red) { emask = b & EDGE_MASK; }
    else { emask = (b >> 15) & EDGE_MASK; }
    return emask;
}

static inline bool contains_edge(board b, edge e)
{
    if ((b & (1 << e)) != 0 || 
        (b & (1 << (e + 15))) != 0) {
        return true;
    }
    return false;
}

static inline bool will_make_triangle(edge_mask emask, edge e)
{
    node i = edge2node_i[e];
    node j = edge2node_j[e];

    for (node k = 0; k < 6; k++) {
        if (k != i && k != j) {
            edge e1 = node2edge[i][k];
            edge e2 = node2edge[k][j];
            if ((emask & (1 << e1)) != 0 &&
                (emask & (1 << e2)) != 0) {
                    return true;
            }
        }
    }
    return false;
}

static inline board place_edge(board b, edge e, bool red)
{
    if (red) return b | (1 << e);
    else     return b | (1 << (e + 15));
}

int search_count = 0, progress_count = 0;

static bool will_wins(board b, bool red, uint32 turn)
{
    if (wins(b, red)) { return true; }
    if (wins(b, !red)) { return false; }
    ++search_count;
    ++progress_count;
    if (progress_count >= 999) {
        progress_count = 0;
        cout << progress_count << ',' << turn << ',' << 
        red << ',' << std::hex << b << std::dec << endl;
    }

    edge_mask emask = board2edge_mask(b, red);
    uint32 win_count = 0;
    for (edge e = 0; e < 15; e++)
    {
        if (contains_edge (b, e)) {
            continue;
        }
        if (will_make_triangle(emask, e)) {
            continue;
        }
        board b1 = place_edge(b, e, red);
        bool w1 = will_wins(b1, !red, turn + 1);
        if (!w1) {
            win_count++;
        }
    }
    if (win_count > 0) {
        set_wins(b, red);
        return true;
    } else {
        set_wins(b, !red);
        return false;
    }
}

edge select_edge_com(board b, bool red, uint32 turn)
{
    edge_mask emask_wins = 0;
    edge_mask emask_lose = 0;
    for (edge e = 0; e < 15; e++)
    {
        if (contains_edge (b, e)) {
            continue;
        }
        board b1 = place_edge(b, e, red);
        if (will_wins(b1, !red, turn)) {
            emask_lose |= (1 << e);
            continue;
        } else {
            emask_wins |= (1 << e);
            continue;
        } 
    }

    uint32 count_wins = popcount(emask_wins);
    uint32 count_lose = popcount(emask_lose);
    uint32 count_select = (count_wins > 0) ? count_wins : count_lose;
    edge_mask emask_select = (count_wins > 0) ? emask_wins : emask_lose;

    uint32 select = rand() % count_select;
    for (edge e = 0; e < 15; e++)
    {
        if (emask_select & (1 << e)) {
            if (select-- == 0) {
                return e;
            }
        }
    }
    assert (0);
    return 0;
}

void com_vs_com()
{
    board b = 0;
    bool red = true;
    for (uint32 turn = 0; turn < 15; turn++, red = !red) {
        edge e = select_edge_com (b, red, turn);
        cout << turn << ':' << (red? "RED," : "BLUE,") << e << endl;
        edge_mask emask = board2edge_mask(b, red);
        if (will_make_triangle (emask, e)) {
            cout << (red? "RED LOSE" : "BLUE LOSE") << endl;
            return;
        }
        b = place_edge(b, e, red);
    }
    cout << "DRAW" << endl;
}

int main (int argc, char** argv)
{
    init_table();
    bool w = will_wins (0, true, 0);
    cout << w << ',' << search_count << endl;

    srand(123);
    for (uint32 iter = 0; iter < 100; iter++) {
        cout << endl;
        com_vs_com();
    }

    return 0;
}