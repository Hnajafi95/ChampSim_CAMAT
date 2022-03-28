#ifndef DRAM_H
#define DRAM_H

#include "memory_class.h"

// DRAM configuration
#define DRAM_CHANNEL_WIDTH 8 // 8B
#define DRAM_WQ_SIZE 64
#define DRAM_RQ_SIZE 64

#define tRP_DRAM_NANOSECONDS  15
#define tRCD_DRAM_NANOSECONDS 15
#define tCAS_DRAM_NANOSECONDS 12.5


// the data bus must wait this amount of time when switching between reads and writes, and vice versa
#define DRAM_DBUS_TURN_AROUND_TIME ((15*CPU_FREQ)/2000) // 7.5 ns 
extern uint32_t DRAM_MTPS, DRAM_DBUS_RETURN_TIME;

// these values control when to send out a burst of writes
#define DRAM_WRITE_HIGH_WM    ((DRAM_WQ_SIZE*7)>>3) // 7/8th
#define DRAM_WRITE_LOW_WM     ((DRAM_WQ_SIZE*3)>>2) // 6/8th
#define MIN_DRAM_WRITES_PER_SWITCH (DRAM_WQ_SIZE*1/4)




/////////////// NVRAM Config//////////////////////////.
#define NVRAM_CHANNEL_WIDTH 8 // 8B
#define NVRAM_WQ_SIZE 128
#define NVRAM_RQ_SIZE 128

#define tRP_NVRAM_NANOSECONDS 22
#define tRCD_NVRAM_NANOSECONDS 22
#define tCAS_NVRAM_NANOSECONDS 15


// the data bus must wait this amount of time when switching between reads and writes, and vice versa
#define NVRAM_DBUS_TURN_AROUND_TIME ((18*CPU_FREQ)/2000) // ?? ns 
extern uint32_t NVRAM_MTPS, NVRAM_DBUS_RETURN_TIME;

// these values control when to send out a burst of writes
#define NVRAM_WRITE_HIGH_WM    ((NVRAM_WQ_SIZE*7)>>3) // 7/8th
#define NVRAM_WRITE_LOW_WM     ((NVRAM_WQ_SIZE*3)>>2) // 6/8th
#define MIN_NVRAM_WRITES_PER_SWITCH (NVRAM_WQ_SIZE*1/4)


void print_dram_config();               
void print_NVram_config();


//////////////////   NVRAM  /////////////////////////////////////

class MEMORY_CONTROLLER_NV : public MEMORY {
  public:
    const string NAME;

    DRAM_ARRAY NVram_array[NVRAM_CHANNELS][NVRAM_RANKS][NVRAM_BANKS];
    uint64_t dbus_cycle_available[NVRAM_CHANNELS], dbus_cycle_congested[NVRAM_CHANNELS], dbus_congested[NUM_TYPES+1][NUM_TYPES+1];
    uint64_t bank_cycle_available[NVRAM_CHANNELS][NVRAM_RANKS][NVRAM_BANKS];
    uint8_t  do_write, write_mode[NVRAM_CHANNELS]; 
    uint32_t processed_writes, scheduled_reads[NVRAM_CHANNELS], scheduled_writes[NVRAM_CHANNELS];
    int fill_level;

    BANK_REQUEST bank_request[NVRAM_CHANNELS][NVRAM_RANKS][NVRAM_BANKS];

    // queues
    PACKET_QUEUE WQ[NVRAM_CHANNELS], RQ[NVRAM_CHANNELS];

    // constructor
    MEMORY_CONTROLLER_NV(string v1) : NAME (v1) {
        for (uint32_t i=0; i<NUM_TYPES+1; i++) {
            for (uint32_t j=0; j<NUM_TYPES+1; j++) {
                dbus_congested[i][j] = 0;
            }
        }
        do_write = 0;
        processed_writes = 0;
        for (uint32_t i=0; i<NVRAM_CHANNELS; i++) {
            dbus_cycle_available[i] = 0;
            dbus_cycle_congested[i] = 0;
            write_mode[i] = 0;
            scheduled_reads[i] = 0;
            scheduled_writes[i] = 0;

            for (uint32_t j=0; j<NVRAM_RANKS; j++) {
                for (uint32_t k=0; k<NVRAM_BANKS; k++)
                    bank_cycle_available[i][j][k] = 0;
            }

            WQ[i].NAME = "NVRAM_WQ" + to_string(i);
            WQ[i].SIZE = NVRAM_WQ_SIZE;
            WQ[i].entry = new PACKET [NVRAM_WQ_SIZE];

            RQ[i].NAME = "NVRAM_RQ" + to_string(i);
            RQ[i].SIZE = NVRAM_RQ_SIZE;
            RQ[i].entry = new PACKET [NVRAM_RQ_SIZE];
        }

        fill_level = FILL_NVRAM;
    };

    // destructor
    ~MEMORY_CONTROLLER_NV() {

    };

    // functions
    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    void schedule(PACKET_QUEUE *queue), process(PACKET_QUEUE *queue),
         update_schedule_cycle(PACKET_QUEUE *queue),
         update_process_cycle(PACKET_QUEUE *queue),
         reset_remain_requests(PACKET_QUEUE *queue, uint32_t channel);

    uint32_t NVram_get_channel(uint64_t address),
             NVram_get_rank   (uint64_t address),
             NVram_get_bank   (uint64_t address),
             NVram_get_row    (uint64_t address),
             NVram_get_column (uint64_t address),
             drc_check_hit (uint64_t address, uint32_t cpu, uint32_t channel, uint32_t rank, uint32_t bank, uint32_t row);

    uint64_t get_bank_earliest_cycle();

    int check_NVram_queue(PACKET_QUEUE *queue, PACKET *packet);
};



// DRAM
class MEMORY_CONTROLLER : public MEMORY {
  public:
    const string NAME;

    DRAM_ARRAY dram_array[DRAM_CHANNELS][DRAM_RANKS][DRAM_BANKS];
    uint64_t dbus_cycle_available[DRAM_CHANNELS], dbus_cycle_congested[DRAM_CHANNELS], dbus_congested[NUM_TYPES+1][NUM_TYPES+1];
    uint64_t bank_cycle_available[DRAM_CHANNELS][DRAM_RANKS][DRAM_BANKS];
    uint8_t  do_write, write_mode[DRAM_CHANNELS]; 
    uint32_t processed_writes, scheduled_reads[DRAM_CHANNELS], scheduled_writes[DRAM_CHANNELS];
    int fill_level;

    BANK_REQUEST bank_request[DRAM_CHANNELS][DRAM_RANKS][DRAM_BANKS];

    // queues
    PACKET_QUEUE WQ[DRAM_CHANNELS], RQ[DRAM_CHANNELS];

    // constructor
    MEMORY_CONTROLLER(string v1) : NAME (v1) {
        for (uint32_t i=0; i<NUM_TYPES+1; i++) {
            for (uint32_t j=0; j<NUM_TYPES+1; j++) {
                dbus_congested[i][j] = 0;
            }
        }
        do_write = 0;
        processed_writes = 0;
        for (uint32_t i=0; i<DRAM_CHANNELS; i++) {
            dbus_cycle_available[i] = 0;
            dbus_cycle_congested[i] = 0;
            write_mode[i] = 0;
            scheduled_reads[i] = 0;
            scheduled_writes[i] = 0;

            for (uint32_t j=0; j<DRAM_RANKS; j++) {
                for (uint32_t k=0; k<DRAM_BANKS; k++)
                    bank_cycle_available[i][j][k] = 0;
            }

            WQ[i].NAME = "DRAM_WQ" + to_string(i);
            WQ[i].SIZE = DRAM_WQ_SIZE;
            WQ[i].entry = new PACKET [DRAM_WQ_SIZE];

            RQ[i].NAME = "DRAM_RQ" + to_string(i);
            RQ[i].SIZE = DRAM_RQ_SIZE;
            RQ[i].entry = new PACKET [DRAM_RQ_SIZE];
        }

        fill_level = FILL_DRAM;
    };

    // destructor
    ~MEMORY_CONTROLLER() {

    };

    // functions
    int  add_rq(PACKET *packet),
         add_wq(PACKET *packet),
         add_pq(PACKET *packet);

    void return_data(PACKET *packet),
         operate(),
         increment_WQ_FULL(uint64_t address);

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address),
             get_size(uint8_t queue_type, uint64_t address);

    void schedule(PACKET_QUEUE *queue), process(PACKET_QUEUE *queue),
         update_schedule_cycle(PACKET_QUEUE *queue),
         update_process_cycle(PACKET_QUEUE *queue),
         reset_remain_requests(PACKET_QUEUE *queue, uint32_t channel);

    uint32_t dram_get_channel(uint64_t address),
             dram_get_rank   (uint64_t address),
             dram_get_bank   (uint64_t address),
             dram_get_row    (uint64_t address),
             dram_get_column (uint64_t address),
             drc_check_hit (uint64_t address, uint32_t cpu, uint32_t channel, uint32_t rank, uint32_t bank, uint32_t row);

    uint64_t get_bank_earliest_cycle();

    int check_dram_queue(PACKET_QUEUE *queue, PACKET *packet);
};
#endif
