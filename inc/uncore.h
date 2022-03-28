#ifndef UNCORE_H
#define UNCORE_H

#include "champsim.h"
#include "cache.h"
#include "dram_controller.h"
//#include "drc_controller.h"

//#define DRC_MSHR_SIZE 48

// uncore
class UNCORE {
  public:

    // LLC
    CACHE LLC{"LLC", LLC_SET, LLC_WAY, LLC_SET*LLC_WAY, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE};

    // DRAM_0
    MEMORY_CONTROLLER DRAM0{"DRAM0"}; 
  // DRAM_1
    MEMORY_CONTROLLER_NV DRAM1{"DRAM1"}; 

    UNCORE(); 
};

extern UNCORE uncore;

#endif
