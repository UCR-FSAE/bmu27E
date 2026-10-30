#include <Arduino.h>
#include "ESP32_CAN.h"

#include "fsm.h"

ESP32_CAN Can;

// CAN messages
static CAN_message_t CAN_outMsg1;
static CAN_message_t CAN_outMsg2;

// ------------- BMS Types -------------
CurrentState    curState;
Inputs          in;
Outputs         out;

void setup() {
    Serial.begin(9600);

}

void loop() {
    // read cell volt, temp, pack current, fault flags from ICs
    // Check for implausible data
    run_fsm(curState, in, out);
    // Send faults if needed
    // Balance cells (if needed)
    // Manage charging 
    // Sends data over CAN
}