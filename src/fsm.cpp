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

void run_fsm(CurrentState& curSt, const Inputs& in, Outputs& out, FaultSummary summary[12], //TODO should I use this approach to pass variables? 
BridgeFaultSummary bridgeSummary,
FaultRegisters registers[12],
BridgeFaultRegisters bridgeRegisters) {
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
                // Poll the BQ 
                BmsStatus_t summaryStatus = readFaultSummary(summary);
                BmsStatus_t bridgeStatus = readBridgeFaultSummary(&bridgeSummary);
                    if (summaryStatus != BMS_OK || bridgeStatus != BMS_OK) {
                        next_state(curSt, FAULT);  // Or handle communication/read errors separately
                        break;
                    }
                
                //check if any of the summaries reports a fault
               if(checkAnyFault(summary,&bridgeSummary)){
                 next_state(curSt, FAULT);
                 break;
               }
            }
            
        case CHARGING:
            break;
        case FAULT:
            break;
    }
}