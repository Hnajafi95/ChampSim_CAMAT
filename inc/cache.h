#ifndef CACHE_H
#define CACHE_H
#include <set>
#include "memory_class.h"

// PAGE
extern uint32_t PAGE_TABLE_LATENCY, SWAP_LATENCY;
#define SET_MAX 1000000

// CACHE TYPE
#define IS_ITLB 0
#define IS_DTLB 1
#define IS_STLB 2
#define IS_L1I  3
#define IS_L1D  4
#define IS_L2C  5
#define IS_LLC  6

// INSTRUCTION TLB
#define ITLB_SET 16
#define ITLB_WAY 8
#define ITLB_RQ_SIZE 16
#define ITLB_WQ_SIZE 16
#define ITLB_PQ_SIZE 0
#define ITLB_MSHR_SIZE 8
#define ITLB_LATENCY 1

// DATA TLB
#define DTLB_SET 16
#define DTLB_WAY 4
#define DTLB_RQ_SIZE 16
#define DTLB_WQ_SIZE 16
#define DTLB_PQ_SIZE 0
#define DTLB_MSHR_SIZE 8
#define DTLB_LATENCY 1

// SECOND LEVEL TLB
#define STLB_SET 128
#define STLB_WAY 12
#define STLB_RQ_SIZE 32
#define STLB_WQ_SIZE 32
#define STLB_PQ_SIZE 0
#define STLB_MSHR_SIZE 16
#define STLB_LATENCY 8

// L1 INSTRUCTION CACHE
#define L1I_SET 64
#define L1I_WAY 8
#define L1I_RQ_SIZE 64
#define L1I_WQ_SIZE 64 
#define L1I_PQ_SIZE 8
#define L1I_MSHR_SIZE 8
#define L1I_LATENCY 1

// L1 DATA CACHE
#define L1D_SET 64
#define L1D_WAY 8
#define L1D_RQ_SIZE 64
#define L1D_WQ_SIZE 64 
#define L1D_PQ_SIZE 32
#define L1D_MSHR_SIZE 16
#define L1D_LATENCY 4

// L2 CACHE
#define L2C_SET 512
#define L2C_WAY 8
#define L2C_RQ_SIZE 32
#define L2C_WQ_SIZE 32
#define L2C_PQ_SIZE 16
#define L2C_MSHR_SIZE 32
#define L2C_LATENCY 10  // 4 (L1I or L1D) + 10 = 14 cycles

// LAST LEVEL CACHE
#define LLC_SET NUM_CPUS*2048
#define LLC_WAY 16
#define LLC_RQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_WQ_SIZE NUM_CPUS*L2C_MSHR_SIZE //48
#define LLC_PQ_SIZE NUM_CPUS*32
#define LLC_MSHR_SIZE NUM_CPUS*64
#define LLC_LATENCY 20  // 4 (L1I or L1D) + 10 + 20 = 34 cycles

extern uint64_t LLC_MSHR;

void print_cache_config();

class CACHE : public MEMORY {
  public:
    uint32_t cpu;
    const string NAME;
    const uint32_t NUM_SET, NUM_WAY, NUM_LINE, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
    uint32_t LATENCY;
    BLOCK **block;
    int fill_level;
    uint32_t MAX_READ, MAX_FILL;
    uint32_t reads_available_this_cycle;
    uint8_t cache_type;
    
    // prefetch stats
    uint64_t pf_requested,
             pf_issued,
             pf_useful,
             pf_useless,
	     pf_late,
             pf_fill;

    // queues
    // PACKET_QUEUE WQ0{NAME + "_WQ0", WQ_SIZE}, // write0 queue
    //              WQ1{NAME + "_WQ1", WQ_SIZE}, // write1 queue

    //              RQ0{NAME + "_RQ0", RQ_SIZE}, // read0 queue
    //              RQ1{NAME + "_RQ1", RQ_SIZE}, // read1 queue

    PACKET_QUEUE WQ{NAME + "_WQ", WQ_SIZE}, // write queue
                 RQ{NAME + "_RQ", RQ_SIZE}, // read0 queue


                 PQ{NAME + "_PQ", PQ_SIZE}, // prefetch queue
                 MSHR{NAME + "_MSHR", MSHR_SIZE}, // MSHR
                 PROCESSED{NAME + "_PROCESSED", ROB_SIZE}; // processed queue
    
    // camat: active cycles + active hit cycles + active miss cycles
    std::set<uint64_t> active_cycles;
    std::set<uint64_t> active_hit_cycles;
    std::set<uint64_t> active_miss_cycles;    
    std::map<uint32_t, set<uint64_t>> active_hit_cycles_per_core;
    std::map<uint32_t, set<uint64_t>> active_miss_cycles_per_core;
    std::map<uint32_t, set<uint64_t>> active_cycles_per_core;
    
    uint64_t total_active_cycles,
             total_active_hit_cycles,
             total_active_miss_cycles;
    
    uint64_t total_active_hit_cycles_per_core[NUM_CPUS],
	     total_active_miss_cycles_per_core[NUM_CPUS],
             total_active_cycles_per_core[NUM_CPUS];             
		
    uint64_t sim_access[NUM_CPUS][NUM_TYPES],
             sim_hit[NUM_CPUS][NUM_TYPES],
             sim_miss[NUM_CPUS][NUM_TYPES],
	     sim_overlap_miss[NUM_CPUS][NUM_TYPES],
             sim_pure_miss[NUM_CPUS][NUM_TYPES],
             roi_access[NUM_CPUS][NUM_TYPES],
             roi_hit[NUM_CPUS][NUM_TYPES],
             roi_miss[NUM_CPUS][NUM_TYPES],
             roi_overlap_miss[NUM_CPUS][NUM_TYPES],
	     roi_pure_miss[NUM_CPUS][NUM_TYPES];

    uint64_t total_miss_latency;
    
    // constructor
    CACHE(string v1, uint32_t v2, int v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8) 
        : NAME(v1), NUM_SET(v2), NUM_WAY(v3), NUM_LINE(v4), WQ_SIZE(v5), RQ_SIZE(v6), PQ_SIZE(v7), MSHR_SIZE(v8) {

        LATENCY = 0;

        // cache block
        block = new BLOCK* [NUM_SET];
        for (uint32_t i=0; i<NUM_SET; i++) {
            block[i] = new BLOCK[NUM_WAY]; 

            for (uint32_t j=0; j<NUM_WAY; j++) {
                block[i][j].lru = j;
            }
        }

	for (uint32_t i=0; i<NUM_CPUS; i++)
	{
		if(active_hit_cycles_per_core.find(i) == active_hit_cycles_per_core.end())
		{
			active_hit_cycles_per_core[i] = set<uint64_t>();
		}
		if(active_miss_cycles_per_core.find(i) == active_miss_cycles_per_core.end())
		{
			active_miss_cycles_per_core[i] = set<uint64_t>();
		}
		if(active_cycles_per_core.find(i) == active_cycles_per_core.end())
		{
			active_cycles_per_core[i] = set<uint64_t>();
		}
	}

        for (uint32_t i=0; i<NUM_CPUS; i++) {
            upper_level_icache[i] = NULL;
            upper_level_dcache[i] = NULL;

            for (uint32_t j=0; j<NUM_TYPES; j++) {
                sim_access[i][j] = 0;
                sim_hit[i][j] = 0;
                sim_miss[i][j] = 0;
		sim_overlap_miss[i][j] = 0;
                sim_pure_miss[i][j] = 0;                
                roi_access[i][j] = 0;
                roi_hit[i][j] = 0;
                roi_miss[i][j] = 0;
                roi_pure_miss[i][j] = 0;
            }
        }

	    total_miss_latency = 0;

        lower_level = NULL;
        extra_interface = NULL;
        fill_level = -1;
        MAX_READ = 1;
        MAX_FILL = 1;

        pf_requested = 0;
        pf_issued = 0;
        pf_useful = 0;
        pf_useless = 0;
        pf_late = 0;
        pf_fill = 0;

    };

    // destructor
    ~CACHE() {
        for (uint32_t i=0; i<NUM_SET; i++)
            delete[] block[i];
        delete[] block;
    };

    // functions
    // int CACHE::sub_add_rq(PACKET_QUEUE read_queue, PACKET_QUEUE write_queue, PACKET *packet);
    // int CACHE::sub_add_wq(PACKET_QUEUE read_queue, PACKET_QUEUE write_queue, PACKET *packet);
    // int CACHE::sub_add_pq(PACKET_QUEUE read_queue, PACKET_QUEUE write_queue, PACKET *packet);

    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    int  check_hit(PACKET *packet),
         invalidate_entry(uint64_t inval_addr),
         check_mshr(PACKET *packet),
         prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, uint32_t prefetch_metadata),
         kpc_prefetch_line(uint64_t base_addr, uint64_t pf_addr, int prefetch_fill_level, int delta, int depth, int signature, int confidence, uint32_t prefetch_metadata);

    void handle_fill(),
         handle_writeback(),
         handle_read(),
         handle_prefetch();

    void add_mshr(PACKET *packet),
         update_fill_cycle(),
         llc_initialize_replacement(),
         update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         llc_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit),
         lru_update(uint32_t set, uint32_t way),
         fill_cache(uint32_t set, uint32_t way, PACKET *packet),
         replacement_final_stats(),
         llc_replacement_final_stats(),
         //prefetcher_initialize(),
         l1d_prefetcher_initialize(),
         l2c_prefetcher_initialize(),
         llc_prefetcher_initialize(),
         prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
         l1d_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type),
         prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr),
         l1d_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         //prefetcher_final_stats(),
         l1d_prefetcher_final_stats(),
         l2c_prefetcher_final_stats(),
         llc_prefetcher_final_stats();

    uint32_t l2c_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
         llc_prefetcher_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in),
         l2c_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in),
         llc_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in);

    void prefetcher_feedback(uint64_t &pref_gen, uint64_t &pref_fill, uint64_t &pref_used, uint64_t &pref_late);
    
    uint32_t get_set(uint64_t address),
             get_way(uint64_t address, uint32_t set),
             find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             llc_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type),
             lru_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type);
};

#endif
