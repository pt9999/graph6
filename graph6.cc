#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <cstring>
using namespace std;

#define popcount __builtin_popcount 

typedef __uint8_t uint8;
typedef __uint32_t uint32;

//  node count
const uint32 N = 6;
//  edge count
const uint32 E = N * (N - 1) / 2;

typedef uint32 node;       // 0..5
typedef uint32 edge;       // 0..14
typedef uint32 edge_set;   // 15bit
typedef uint32 board;      // 30bit
const uint32 BOARD_COUNT = (1 << (E * 2));
const uint32 EDGE_MASK = (1 << E) - 1;
uint8 red_wins_table[BOARD_COUNT >> 3];
uint8 blue_wins_table[BOARD_COUNT >> 3];

edge node2edge[N][N];
node edge2node_i[E];
node edge2node_j[E];
static void init_table ()
{
    //cout << std::hex << BOARD_COUNT << std::dec << endl;
    //cout << std::hex << EDGE_MASK << std::dec << endl;

    memset(red_wins_table, 0, sizeof(red_wins_table));
    memset(blue_wins_table, 0, sizeof(blue_wins_table));

    //edge node_i2edge[N] = { 0, 5, 9, 12, 14, 15 };
    edge node_i2edge[N];
    node_i2edge[0] = 0;
    for (uint32 i = 1; i < N; i++) {
        node_i2edge[i] = node_i2edge[i-1] + (N - i);
    }

    for (uint32 i = 0; i < N; i++) {
        for (uint32 j = i + 1; j < N; j++) {
            edge e = node_i2edge[i] + (j - i - 1);
            assert (e < E);
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
    return (red_wins_table[b >> 3] & (1 << (b & 7))) != 0;
}

static inline bool blue_wins(board b)
{
    return (blue_wins_table[b >> 3] & (1 << (b & 7))) != 0;
}

static inline bool wins(board b, bool red)
{
    if (red) return red_wins(b);
    else return blue_wins(b);
}

static inline void set_wins(board b, bool red)
{
    if (red) {
        red_wins_table[b >> 3] |= (1 << (b & 7));
    } else {
        blue_wins_table[b >> 3] |= (1 << (b & 7));
    }
}

static void save_wins_table()
{
    ofstream ofs ("table.dat", ios::binary);
    ofs.write((char*) red_wins_table, sizeof(red_wins_table));
    ofs.write((char*) blue_wins_table, sizeof(blue_wins_table));
}

/*
static void wins_test()
{
    assert (! wins (12345, true));
    assert (! wins (6789, false));
    set_wins(12345, true);
    set_wins(6789, false);

    assert (wins (12345, true));
    assert (wins (6789, false));
    exit (0);
}
*/

static inline edge_set get_board_edges(board b, bool red)
{
    edge_set edges;
    if (red) { edges = b & EDGE_MASK; }
    else { edges = (b >> E) & EDGE_MASK; }
    return edges;
}

static inline bool board_has_edge(board b, edge e)
{
    edge_set edges = (b | (b >> E)) & EDGE_MASK;
    return (edges & (1 << e)) != 0;
}

static inline bool will_make_triangle(edge_set edges, edge e)
{
    node i = edge2node_i[e];
    node j = edge2node_j[e];

    assert ((edges & (1 << e)) == 0);

    for (node k = 0; k < N; k++) {
        if (k != i && k != j) {
            edge e1 = node2edge[i][k];
            edge e2 = node2edge[k][j];
            if ((edges & (1 << e1)) != 0 &&
                (edges & (1 << e2)) != 0) {
                    return true;
            }
        }
    }
    return false;
}

static inline board board_add_edge(board b, edge e, bool red)
{
    if (red) return b | (1 << e);
    else     return b | (1 << (e + E));
}

uint32 search_count = 0, progress_count = 0;

static bool will_win(board b, bool red, uint32 turn)
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

    edge_set my_edges = get_board_edges(b, red);
    uint32 win_count = 0;
    for (edge e = 0; e < E; e++)
    {
        if (board_has_edge (b, e)) {
            continue;
        }
        if (will_make_triangle(my_edges, e)) {
            continue;
        }
        board b1 = board_add_edge(b, e, red);
        bool w1 = will_win(b1, !red, turn + 1);
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
    edge_set my_edges = get_board_edges(b, red);
    edge_set win_edges = 0;
    edge_set lose_edges = 0;
    for (edge e = 0; e < E; e++)
    {
        if (board_has_edge (b, e)) {
            continue;
        }
        if (will_make_triangle(my_edges, e)) {
            lose_edges |= (1 << e);
            continue;
        }
        board b1 = board_add_edge(b, e, red);
        if (will_win(b1, !red, turn)) {
            lose_edges |= (1 << e);
            continue;
        } else {
            win_edges |= (1 << e);
            continue;
        } 
    }

    uint32 win_count = popcount(win_edges);
    uint32 lose_count = popcount(lose_edges);
    cout << "W:" << win_count << ':' << std::hex << win_edges << std::dec << endl;
    cout << "L:" << lose_count << ':' << std::hex << lose_edges << std::dec << endl;

    uint32 selectable_count = (win_count > 0) ? win_count : lose_count;
    edge_set selectable_edges = (win_count > 0) ? win_edges : lose_edges;

    uint32 selection = rand() % selectable_count;
    for (edge e = 0; e < E; e++)
    {
        if (selectable_edges & (1 << e)) {
            if (selection-- == 0) {
                return e;
            }
        }
    }
    assert (0);
    return 0;
}

bool com_vs_com()
{
    board b = 0;
    bool red = true;
    for (uint32 turn = 0; turn < E; turn++, red = !red) {
        edge e = select_edge_com (b, red, turn);
        cout << turn << ':' << (red? "RED " : "BLUE")
         << "," << edge2node_i[e]
         << "," << edge2node_j[e]
         << " (" << e << ")"
         << endl;
        edge_set edges = get_board_edges(b, red);
        if (will_make_triangle (edges, e)) {
            cout << (red? "RED" : "BLUE") << " LOSE" << endl;
            return !red;    // winner
        }
        b = board_add_edge(b, e, red);
    }
    cout << (red? "RED" : "BLUE") << " LOSE" << endl;
    return !red;
}

int main (int argc, char** argv)
{
    init_table();
    //wins_test();
    bool red = true;
    bool red_will_win = will_win (0, red, 0);
    cout << red_will_win << ',' << std::dec << search_count << endl;
    cout << (!red_will_win ? "RED" : "BLUE") << " WILL LOSE"  << endl;
    //save_wins_table();

    srand(123);
    for (uint32 iter = 0; iter < 100; iter++) {
        cout << endl;
        bool winner = com_vs_com();
        if (red_will_win) assert (winner == true);
        else assert (winner == false);
    }

    return 0;
}