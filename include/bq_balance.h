// bqUartInit(); bq_wake(); bq_auto_adress(); bring-up first
// bq_auto_balance(masks, CB_TIMER_60s); start
// bq_balance_print_status(); check
// bq_balance_stop(); stop


#pragma once
#include <Arduino.h>
#include "bq_comm.h"

#ifndef CELLS_PER_DEVICE
#define CELLS_PER_DEVICE 10 // series cell groups
#endif

//----- registers ----
#define REG_CB_CELL1_CTRL    0x0327
#define REG_VCBDONE_THRESH   0x032A    // stop balancing once cell drops to this voltage
#define REG_BAL_CTRL1        0x032E    // auto-balance switching period
#define REG_BAL_STAT         0x0532    // balancing status

//----- BAL_STAT bits ---
#define BAL_STAT_CB_DONE      0x01    //all cell timers finsihed
#define BAL_STAT_ABORTFLT     0x04    // stopped cuz of fault
#define BAL_STAT_CB_RUN       0x08    // balancing is running
#define BAL_STAT_CB_INPAUSE   0x020   // paused
#define BAL_STAT_OT_PAUSE_DET 0x40    // puased for over temp
#define BAL_STAT_INVALID_CBCONF 0x80  // config rejected by chip

// --- timer codes for CB_CELLx_CTRL (how long each cell balances) ---
#define CB_TIMER_OFF    0x00
#define CB_TIMER_10S    0x01
#define CB_TIMER_30S    0x02
#define CB_TIMER_60S    0x03
#define CB_TIMER_5MIN   0x04
#define CB_TIMER_10MIN  0x05
#define CB_TIMER_20MIN  0x06
#define CB_TIMER_30MIN  0x07

// --- duty codes for BAL_CTRL1 (switching period) --- 
#define BAL_DUTY_5S    0x00
#define BAL_DUTY_10S   0x01
#define BAL_DUTY_30S   0x02
#define BAL_DUTY_60S   0x03

// --- settings --- 
#define BAL_DUTY_DEFAULT    BAL_DUTY_10S
#define BAL_VCBDONE_DEFAULT 0x00
#define BAL_USE_OTCB    0


bool bq_auto_balance(const uint16_t maks[NUM_STACK_DEVICES], uint8_t timerCode);
// returns false if any device with cells selected is not balancing

bool bq_auto_balanceAll(uint16_t mask, uint8_t timerCode);
// same timer on same cells on every device

bool bq_balanceStop();
// stop balancing on every device

bool bq_balancePause(bool pause);
// pause before reading cell volts

bool bq_balance_PrintStatus();
// reading and printing
// returns false on missing replies

bool bq_balanceSelect(const uint16_t cell_mv[NUM_STACK_DEVICES][CELLS_PER_DEVICE],
                        uint16_t threshold_mv, uint16_t minCell_mv, 
                        uint16_t masks[NUM_STACK_DEVICES]);
// builds masks from cell volts
// balances cells at or above minCell_mv