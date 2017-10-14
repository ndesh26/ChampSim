#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <vector>
#include <bits/stdc++.h>

#define LLC_SET 8096
#define LLC_WAY 16
#define LOG2_BLOCK_SIZE 6

// CACHE ACCESS TYPE
#define LOAD      0
#define RFO       1
#define PREFETCH  2
#define WRITEBACK 3

using namespace std;

struct access {
    uint64_t full_addr;
    uint32_t type;
};

vector <uint64_t> access_hist_global;
vector <uint64_t> access_hist[LLC_SET];
uint64_t cache[LLC_SET][LLC_WAY];
uint64_t hist_index[LLC_SET];

int find_victim(uint64_t set, uint64_t full_addr) {
    vector <int> remaining;

    for (int i = 0; i < LLC_WAY; ++i) {
        if(cache[set][i] == 0)
            return i;
    }
    for (int i = 0; i < LLC_WAY; ++i) {
        remaining.push_back(i);
    }

    //cout << "access_hist size: " << access_hist[set].size() << endl;
    for(int i = hist_index[set]; i < access_hist[set].size(); ++i) {
        if (remaining.size() == 1) {
            return remaining[0];
        }

        for(vector<int>::iterator it = remaining.begin(); it < remaining.end(); ++it) {
            if (cache[set][*it] == access_hist[set].at(i)) {
                remaining.erase(it);
                break;

            }
        } 
    }

    return remaining[0];
}

int check_block(uint64_t set, uint64_t full_addr) {
    for(int i = 0; i < LLC_WAY; i++) {
        if (cache[set][i] == full_addr)
            return 1;
    }

    return 0;
}

void read_data(FILE *access) {
    struct access curr;
    uint64_t set;

    while(fread(&curr, sizeof(struct access), 1, access)) {
        if (curr.type == LOAD || curr.type == RFO) {
            set = (curr.full_addr >> LOG2_BLOCK_SIZE) % LLC_SET;
            access_hist_global.push_back(curr.full_addr);
            access_hist[set].push_back(curr.full_addr);
        }
    }
}

int main(int argc, char *argv[]) {
    FILE *access;
    uint64_t accesses = 0;
    uint64_t hits = 0;
    uint64_t set;
    int way;

    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <access_file>" << endl;
        exit(1);
    }

    access = fopen(argv[1], "r");
    read_data(access);

    for (vector<uint64_t>::iterator it = access_hist_global.begin(); it != access_hist_global.end(); ++it) {
        accesses++;
        set = ((*it) >> LOG2_BLOCK_SIZE) % LLC_SET;
        hist_index[set]++;
        if (check_block(set, *it))
            hits++;
        else {
            way = find_victim(set, *it);
            cache[set][way] = (*it);
        }
    }

    cout << argv[1] <<","<< accesses << "," << accesses-hits << endl;
}
