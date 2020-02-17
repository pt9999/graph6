#include <iostream>
#include <cassert>

using namespace std;

typedef __uint8_t uint8;
typedef __uint32_t uint32;
typedef uint32 node;       // 0..5
typedef uint32 edge;       // 0..14
typedef uint32 edge_mask;  // 15bit
typedef uint32 board;      // 30bit
const int BOARD_COUNT = (1 << 30);
const int EDGE_MASK = (1 << 15) - 1;
uint8 black_wins_table[BOARD_COUNT >> 3];
uint8 white_wins_table[BOARD_COUNT >> 3];

edge node2edge[6][6];
node edge2node_i[15];
node edge2node_j[15];
void init_edge2node ()
{
    edge node_i2edge[6] = { 0, 5, 9, 12, 14, 15 };

    for (int i = 0; i < 6; i++) {
        for (int j = i + 1; j < 6; j++) {
            edge e = node_i2edge[i] + (j - i - 1);
            //edge e = node2edge(i, j);
            assert (e < 15);
            node2edge[i][j] = e;
            node2edge[j][i] = e;
            edge2node_i[e] = i;
            edge2node_j[e] = j;
            cout << i << ',' << j << ',' << e << endl;
        }
    }
}


bool black_wins(board b)
{
    return (black_wins_table[b >> 3] & (1 << (b&7))) != 0;
}

bool white_wins(board b)
{
    return (white_wins_table[b >> 3] & (1 << (b&7))) != 0;
}

bool wins(board b, bool black)
{
    if (black) return black_wins(b);
    else return white_wins(b);
}

void set_wins(board b, bool black)
{
    if (black) {
        black_wins_table[b >> 3] |= (1 << (b&7));
    } else {
        white_wins_table[b >> 3] |= (1 << (b&7));
    }
}

edge_mask board2edge_mask(board b, bool black)
{
    edge_mask emask;
    if (black) { emask = b & EDGE_MASK; }
    else { emask = (b >> 15) & EDGE_MASK; }
    return emask;
}

bool can_place(edge_mask emask, edge e)
{
/*    if ((b & (1 << e)) != 0 || (b & (1 << (e + 15)))) {
        return false;
    }*/
    node i = edge2node_i[e];
    node j = edge2node_j[e];

    for (node k = 0; k < 6; k++) {
        if (k != i && k != j) {
            edge e1 = node2edge[i][k];
            edge e2 = node2edge[k][j];
            if ((emask & (1 << e1)) == 0 ||
                (emask & (1 << e2)) == 0) {
                    return true;
            }
        }
    }
    return false;
}

board place(board b, edge e, bool black)
{
    if (black) return b | (1 << e);
    else       return b | (1 << (e + 15));
}

int search_count = 0, progress_count = 0;

bool will_wins(board b, bool black, uint32 level)
{
    if (wins(b, black)) { return true; }
    if (wins(b, !black)) { return false; }
    ++search_count;
    ++progress_count;
    if (progress_count >= 1000) {
        progress_count = 0;
        cout << progress_count << ',' << level << ',' << 
        black << ',' << std::hex << b << std::dec << endl;
    }

    edge_mask emask = board2edge_mask(b, black);
    uint32 win_count = 0;
    for (edge e = 0; e < 15; e++)
    {
        if ((b & (1 << e)) != 0 || (b & (1 << (e + 15)))) {
            continue;
        }
        if (! can_place(emask, e)) {
            continue;
        }
        board b1 = place(b, e, black);
        bool w1 = will_wins(b1, !black, level + 1);
        if (!w1) {
            win_count++;
        }
    }
    if (win_count > 0) {
        set_wins(b, black);
        return true;
    } else {
        set_wins(b, !black);
        return false;
    }
}

int main (int argc, char** argv)
{
    init_edge2node();
    bool w = will_wins (0, true, 0);
    cout << w << ',' << search_count << endl;
    return 0;
}