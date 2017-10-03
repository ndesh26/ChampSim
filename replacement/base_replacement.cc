#include "cache.h"
#include <inttypes.h>
#include <vector>
#include <bits/stdc++.h>
#define MAX_REUSE_DIST 8000

struct access {
    uint64_t full_addr;
    uint32_t type;
};

FILE *outfile[NUM_CPUS];
FILE *reuse_csv[NUM_CPUS];
vector <uint64_t> access_hist[NUM_CPUS][L2C_SET];
uint64_t reuse_dist[NUM_CPUS][MAX_REUSE_DIST];
unordered_map <uint64_t, uint64_t> access_map[NUM_CPUS];
uint64_t curr_index[NUM_CPUS][L2C_SET];


// initialize replacement state
void CACHE::l2c_initialize_replacement(char *tracefile)
{
    char outfilename[1024];
    char *last_slash = strrchr(tracefile, '/')+1;
    char * first_dot = strchr(tracefile, '.');
    char file[1024];
    snprintf(file, first_dot - last_slash + 1, "%s", last_slash);

    snprintf(outfilename, 102, "/data/ndesh/UGP/cloudsuite_access/L2C/%s.access", file);
    outfile[cpu] = fopen(outfilename, "wb");
    if (!outfile[cpu]) {
        perror("Error: ");
        assert(0);
    }

    snprintf(outfilename, 102, "/data/ndesh/UGP/cloudsuite_access/L2C/%s.csv", file);
    reuse_csv[cpu] = fopen(outfilename, "wb");
    if (!reuse_csv[cpu]) {
        perror("Error: ");
        assert(0);
    }
}

uint32_t CACHE::find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // baseline LRU replacement policy for other caches 
    return lru_victim(cpu, instr_id, set, current_set, ip, full_addr, type); 
}

void CACHE::update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    if (cache_type == IS_L2C) {
        uint64_t block_addr = (full_addr  >> LOG2_BLOCK_SIZE);
        access_hist[cpu][set].push_back(block_addr);
        curr_index[cpu][set]++;

        if (access_map[cpu].find(block_addr) != access_map[cpu].end()) {
            access_hist[cpu][set][access_map[cpu][(block_addr)]] = (uint64_t)0;
            unsigned int curr_reuse = 0;
            for (uint64_t i = access_map[cpu][(block_addr)]; i < curr_index[cpu][set]-1; i++) {
                if (access_hist[cpu][set].at(i) != 0)
                    curr_reuse += 1;
            }
            if (curr_reuse < MAX_REUSE_DIST) {
                reuse_dist[cpu][curr_reuse]++;
            }
        }
        access_map[cpu][(block_addr)] = curr_index[cpu][set]-1;
        struct access curr;
        curr.full_addr = full_addr;
        curr.type = type;
        fwrite(&curr, sizeof(struct access), 1, outfile[cpu]);
    }

    if (type == WRITEBACK) {
        if (hit) // wrietback hit does not update LRU state
            return;
    }

    return lru_update(set, way);
}

uint32_t CACHE::lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    uint32_t way = 0;

    // fill invalid line first
    for (way=0; way<NUM_WAY; way++) {
        if (block[set][way].valid == false) {

            DP ( if (warmup_complete[cpu]) {
            cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " invalid set: " << set << " way: " << way;
            cout << hex << " address: " << (full_addr>>LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
            cout << dec << " lru: " << block[set][way].lru << endl; });

            break;
        }
    }

    // LRU victim
    if (way == NUM_WAY) {
        for (way=0; way<NUM_WAY; way++) {
            if (block[set][way].lru == NUM_WAY-1) {

                DP ( if (warmup_complete[cpu]) {
                cout << "[" << NAME << "] " << __func__ << " instr_id: " << instr_id << " replace set: " << set << " way: " << way;
                cout << hex << " address: " << (full_addr>>LOG2_BLOCK_SIZE) << " victim address: " << block[set][way].address << " data: " << block[set][way].data;
                cout << dec << " lru: " << block[set][way].lru << endl; });

                break;
            }
        }
    }

    if (way == NUM_WAY) {
        cerr << "[" << NAME << "] " << __func__ << " no victim! set: " << set << endl;
        assert(0);
    }

    return way;
}

void CACHE::lru_update(uint32_t set, uint32_t way)
{
    // update lru replacement state
    for (uint32_t i=0; i<NUM_WAY; i++) {
        if (block[set][i].lru < block[set][way].lru) {
            block[set][i].lru++;
        }
    }
    block[set][way].lru = 0; // promote to the MRU position
}

void CACHE::replacement_final_stats()
{
    if (cache_type == IS_L2C) {
        for(int i = 0; i < MAX_REUSE_DIST; i++) {
            fprintf(reuse_csv[cpu], "%d,%" PRIu64 "\n", i, reuse_dist[cpu][i]);
        }
        fclose(outfile[cpu]);
        fclose(reuse_csv[cpu]);
    }
}

#ifdef NO_CRC2_COMPILE
void InitReplacementState()
{
    
}

uint32_t GetVictimInSet (uint32_t cpu, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t paddr, uint32_t type)
{
    return 0;
}

void UpdateReplacementState (uint32_t cpu, uint32_t set, uint32_t way, uint64_t paddr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    
}

void PrintStats_Heartbeat()
{
    
}

void PrintStats()
{

}
#endif
