#include "fsm.h"
#include "bq_comm.h"

/*
    BOOT_FAULT,     // Just powered on, init variables
    INIT,           // wake up bq and configure 
    RUNNING,        // running main logic order
    CHARGING,       // interfacing with charger
    FAULT           // fault latched
*/

/*
    resets state_ticks
*/
void next_state(CurrentState& curSt, State s) {
    curSt.state = s;
    curSt.state_ticks = 0;
}

void run_fsm(CurrentState& curSt, const Inputs& in, Outputs& out) {
    out = {};

    switch (curSt.state) {
        case BOOT_FAULT:
            break;
        case INIT:
            break;
        case RUNNING:
            if (commLost()){ 
             next_state(curSt, FAULT);
             break;
            }
            if (++curSt.state_ticks >= 100) {  // TODO check fsm ticks -- 100 × 10 ms = 1 second
                curSt.state_ticks = 0;
                // Poll the BQ here
                
        
            }
            
        case CHARGING:
            break;
        case FAULT:
            break;
    }
}