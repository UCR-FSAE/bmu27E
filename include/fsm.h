#pragma once 

#include "bms_types.h"

enum State {
    BOOT_FAULT,     // Just powered on, init variables
    INIT,           // wake up bq and configure 
    RUNNING,        // running main logic order
    CHARGING,       // interfacing with charger
    FAULT           // fault latched
};

/*
    Struct to track state of vcu
    state_ticks: gets incremented every 10ms, 
        used to track how long fsm has been in a state,
        used by RTD and INVERTER_INIT states
*/
struct CurrentState {
    State    state = BOOT_FAULT;
    uint16_t state_ticks = 0;
};

void next_state(CurrentState& curSt, State s);
void run_fsm(CurrentState& curSt, const Inputs& in, Outputs& out);
