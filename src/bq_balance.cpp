#include "bq_balance.h"

static BqResponse balResp;
static bool balancing = false;

// BAL_CTRL2 used everytime to start/resume
static uint8_t balOptions() {
    uint8_t v = BAL_CTRL2_AUTO_BAL | BAL_CTRL2_FLTSTOP_EN;
    if(BAL_USE_OTCB){v |= BAL_CTRL2_OTCB_EN;}
    return v;
}

// helper functions with cell timers

static bool writeTimers(uint8_t msgType, uint8_t dev, uint16_t mask,uint8_t timerCode){
    uint8_t buf[CELLS_PER_DEVICE];
    for(int i = 1; i <= CELLS_PER_DEVICE; i++){
        uint8_t t = (mask & (1u << (i - 1))) ? timerCode : CB_TIMER_OFF;
        buf[CELLS_PER_DEVICE - i] = t;
    }
    uint16_t start = REG_CB_CELL1_CTRL - (CELLS_PER_DEVICE - 1);

    bool ok = true;
    for(int off = 0; off < CELLS_PER_DEVICE; off += 8){
        uint8_t len = (CELLS_PER_DEVICE - off > 8) ? 8 : (uint8_t)(CELLS_PER_DEVICE - off);
        ok &= bqWrite(msgType, dev, start + off, buf + off, len);
    }
    return ok;
}

// helper funciton to check it started
static bool verifyStart(const uint16_t masks[NUM_STACK_DEVICES]){
    delay(5);
    int n = bqRead(STACK_READ_VAL, 0, REG_BAL_STAT, 1, NUM_STACK_DEVICES, &balResp, 20);
    bool ok = (n == NUM_STACK_DEVICES);

    for(int i = 1; i <= NUM_STACK_DEVICES; i++){
        uint16_t mask = masks[i - 1];
        if(!balResp.valid[i]){
            Serial.printf("[BAL] device %d: NO REPLY\n", i);
            ok = false;
            continue;
        }
        uint8_t s = balResp.data[i][0];
        bool running = s & (BAL_STAT_CB_RUN | BAL_STAT_CB_INPAUSE);
        bool bad = s & (BAL_STAT_INVALID_CBCONF | BAL_STAT_ABORTFLT);

        if (mask == 0){
            Serial.printf("[BAL] device %d: no cells selected\n", i);
        } else if (running && !bad){
            Serial.printf("[BAL] device %d: OK cells 0x%03x (BAL_STAT 0x%02X)\n", i, mask, s);
        } else {
            Serial.printf("[BAL] device %d: NOT RUNNING cells 0x%03X  (BAL_STAT 0x%02X)%s%s\n",
                i, mask, s,
                (s & BAL_STAT_INVALID_CBCONF) ? " INVALID_CBCONF" : "",
                (s & BAL_STAT_ABORTFLT) ? " ABORTFLT (clear faults?)" : "");
            ok = false;
        }
    }
    return ok;
}

// --- main functions --- 
bool bq_auto_balance(const uint16_t masks[NUM_STACK_DEVICES], uint8_t timerCode){
    bool any = false;
    for(int i = 0; i < NUM_STACK_DEVICES; i++) any |= (masks[i] != 0);
    if(!any || timerCode == CB_TIMER_OFF){    // dont have anything to balance
        bq_balanceStop();
        return true;
    }
    // switching period & volt based stop
    bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL1, BAL_DUTY_DEFAULT);
    bqWrite1(STACK_WRITE_VAL, 0, REG_VCBDONE_THRESH, BAL_VCBDONE_DEFAULT);

    // each device get own cell timers
    for(uint8_t i = 1; i <= NUM_STACK_DEVICES; i++){
        writeTimers(SINGLE_WRITE_VAL, i, masks[i - 1], timerCode);
    }

    // auto mode
    bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL2, balOptions() | BAL_CTRL2_BAL_GO);

    // read and confirm devices are running
    bool ok = verifyStart(masks);
    balancing = ok;
    return ok;
}

// same cells on every device (one stack write) 
bool bq_auto_balanceAll(uint16_t mask, uint8_t timerCode){
    uint16_t masks[NUM_STACK_DEVICES];
    for(int i = 0; i < NUM_STACK_DEVICES; i++) masks[i] = mask;
    if(mask == 0 || timerCode == CB_TIMER_OFF){ bq_balanceStop(); return true; }

    // switching period & volt based stop
    bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL1, BAL_DUTY_DEFAULT);
    bqWrite1(STACK_WRITE_VAL, 0, REG_VCBDONE_THRESH, BAL_VCBDONE_DEFAULT);

    // same cell timers on every device
    writeTimers(STACK_WRITE_VAL, 0, mask, timerCode);

    // auto mode
    bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL2, balOptions() | BAL_CTRL2_BAL_GO);

    // read and confirm devices are running
    bool ok = verifyStart(masks);
    balancing = ok;
    return ok;
}

// stop all balancing
bool bq_balanceStop(){
    // all timers to 0 on every device, then BAL_GO to apply
    bool ok = writeTimers(STACK_WRITE_VAL, 0, 0x0000, CB_TIMER_OFF);
    ok &= bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL2, BAL_CTRL2_BAL_GO);
    balancing = false;
    return ok;
}

bool bq_balance_pause(bool pause){
    uint8_t v = balOptions();
    if(pause) v |= BAL_CTRL2_CB_PAUSE;
    return bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL2, v);
}

bool bq_balance_PrintStatus(){
    int n = bqRead(STACK_READ_VAL, 0, REG_BAL_STAT, 1, NUM_STACK_DEVICES, &balResp, 20);
    Serial.printf("[BAL] status: %d/%d replies\n", n, NUM_STACK_DEVICES);

    for(int i = 1; i <= NUM_STACK_DEVICES; i++){
        if(!balResp.valid[i]){Serial.printf("dev %2d: NO REPLY\n", i); continue;}
        uint8_t s = balResp.data[i][0];
        Serial.printf("dev %2d: 0x%02X %s%s%s%s%s%s\n", i, s,
                      (s & BAL_STAT_CB_RUN)         ? "RUN "      : "",
                      (s & BAL_STAT_CB_DONE)        ? "DONE "     : "",
                      (s & BAL_STAT_CB_INPAUSE)     ? "PAUSED "   : "",
                      (s & BAL_STAT_OT_PAUSE_DET)   ? "OT_PAUSE " : "",
                      (s & BAL_STAT_ABORTFLT)       ? "ABORTFLT " : "",
                      (s & BAL_STAT_INVALID_CBCONF) ? "INVALID "  : "");
    }
    return n == NUM_STACK_DEVICES;
}

bool bq_balanceSelect(const uint16_t cell_mv[NUM_STACK_DEVICES][CELLS_PER_DEVICE],
                       uint16_t threshold_mv, uint16_t minCell_mv,
                       uint16_t masks[NUM_STACK_DEVICES]) {
    uint16_t lowest = 0xFFFF;
    for (int d = 0; d < NUM_STACK_DEVICES; d++)
        for (int c = 0; c < CELLS_PER_DEVICE; c++)
            if (cell_mv[d][c] < lowest) lowest = cell_mv[d][c];

    bool any = false;
    for (int d = 0; d < NUM_STACK_DEVICES; d++) {
        masks[d] = 0;
        for (int c = 0; c < CELLS_PER_DEVICE; c++) {
            uint16_t v = cell_mv[d][c];
            if (v >= minCell_mv && v > lowest + threshold_mv) {
                masks[d] |= (1u << c);                 
                any = true;
            }
        }
    }
    return any;
}