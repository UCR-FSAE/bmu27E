// // bq_init.cpp — follows BQ79616 datasheet Table 7-19 (Auto-Addressing) with a
// // BQ79600 bridge. Cross-check with TI's Software Design Reference.
// #include "bq_init.h"
// #include "bq_reqs.h"

// // ---- Functions from bq79616.cpp (no header, so declared here) ----
// // Keep these matching bq79616.cpp.
// #define SINGLE_WRITE_VAL   0x90
// #define STACK_WRITE_VAL    0xB0
// #define BROAD_READ_VAL     0xC0
// #define BROAD_WRITE_VAL    0xD0
// #define BQ_MAX_DEVICES     13
// #define BQ_MAX_RESP_BYTES  128

// struct BqResponse {
//   uint8_t data[BQ_MAX_DEVICES][BQ_MAX_RESP_BYTES];
//   bool    valid[BQ_MAX_DEVICES];
//   int     num_valid;
// };

// void wakePing();
// bool bqWrite(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t data_len);
// bool bqWrite1(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t value);
// int  bqRead(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t nbytes,
//             int expected_frames, BqResponse *out, uint32_t timeout_ms);
// // ------------------------------------------------------------------

// // static BqResponse resp;

// // static_assert(TOTAL_DEVICES <= BQ_MAX_DEVICES, "Raise BQ_MAX_DEVICES (here and in bq79616.cpp)");
// // static bool balancing = false;

// // ------------------------------------------------------------------ wake

// void bq_wake() {
//   // 1) Wake ping to the BQ79600 (TX held low). Twice, as in TI's sample code.
//   wakePing();
//   wakePing();

//   // 2) Bridge sends a wake tone up the daisy chain to the 616s.
//   bqWrite1(SINGLE_WRITE_VAL, 0, REG_CONTROL1, CONTROL1_SEND_WAKE);
//   // delay(STACK_WAKE_MS_PER_DEV * NUM_STACK_DEVICES);
// }

// // -------------------------------------------------------- auto-addressing

// // bool bq_auto_address() {
// //   // Step 1: dummy broadcast write 0x00 to ECC_DATA1..8 (sync DLL, write direction)
// //   uint8_t zeros[8] = {0};
// //   bqWrite(BROAD_WRITE_VAL, 0, REG_OTP_ECC_DATAIN1, zeros, 8);
// //   delay(1);

// //   // Step 2: enable auto-addressing
// //   bqWrite1(BROAD_WRITE_VAL, 0, REG_CONTROL1, CONTROL1_ADDR_WR);

// //   // Step 3: broadcast consecutive addresses. Bridge = 0, 616s = 1..N.
// //   for (uint8_t a = 0; a < TOTAL_DEVICES; a++) {
// //     bqWrite1(BROAD_WRITE_VAL, 0, REG_DIR0_ADDR, a);
// //   }

// //   // Step 4 (option 2): all stack devices, then bridge and top of stack.
// //   bqWrite1(BROAD_WRITE_VAL, 0, REG_COMM_CTRL, COMM_CTRL_STACK_DEV);
// //   bqWrite1(SINGLE_WRITE_VAL, 0, REG_COMM_CTRL, 0x00);   // bridge (verify for 600)
// //   bqWrite1(SINGLE_WRITE_VAL, NUM_STACK_DEVICES, REG_COMM_CTRL,
// //             COMM_CTRL_STACK_DEV | COMM_CTRL_TOP_STACK);

// //   // Step 5: dummy broadcast read of ECC_DATA1..8 (sync DLL, read direction).
// //   // Missing responses here are normal.
// //   bqRead(BROAD_READ_VAL, 0, REG_OTP_ECC_DATAIN1, 8, TOTAL_DEVICES, &resp, 20);

// //   // Step 7: read back DIR0_ADDR from every device
// //   int n = bqRead(BROAD_READ_VAL, 0, REG_DIR0_ADDR, 1, TOTAL_DEVICES, &resp, 20);
// //   bool ok = (n == TOTAL_DEVICES);
// //   for (int a = 0; a < TOTAL_DEVICES; a++) {
// //     bool good = resp.valid[a] && (resp.data[a][0] & 0x3F) == a;
// //     Serial.printf("[BQ] device %d: %s\n", a, good ? "OK" : "MISSING/WRONG");
// //     ok &= good;
// //   }

// //   // Step 8: DLL sync can leave comm faults latched — clear them.
// //   uint8_t ff[2] = {0xFF, 0xFF};
// //   bqWrite(STACK_WRITE_VAL, 0, REG_FAULT_RST1, ff, 2);       // FAULT_RST1 + FAULT_RST2
// //   bqWrite1(SINGLE_WRITE_VAL, 0, REG_FAULT_RST_600, 0xFF);   // bridge

// //   return ok;
// // }

// // bool bq_init() {
// //   bq_wake();
// //   return bq_auto_address();
// // }

// // // ---------------------------------------------------------------- balancing

// // static bool write_cb_timers(uint8_t addr, uint8_t type, const uint8_t* timers /* cell1..cellN */) {
// //   // Registers run CB_CELL16_CTRL (low addr) ... CB_CELL1_CTRL (high addr),
// //   // so cells 1..N sit at CB_CELL1_CTRL-(N-1) ... CB_CELL1_CTRL.
// //   uint8_t buf[CELLS_PER_DEVICE];
// //   for (int c = 0; c < CELLS_PER_DEVICE; c++) buf[CELLS_PER_DEVICE - 1 - c] = timers[c];
// //   uint16_t start = REG_CB_CELL1_CTRL - (CELLS_PER_DEVICE - 1);
// //   bool ok = true;
// //   for (int off = 0; off < CELLS_PER_DEVICE; off += 8) {   // max 8 data bytes per frame
// //     uint8_t len = (CELLS_PER_DEVICE - off) > 8 ? 8 : (uint8_t)(CELLS_PER_DEVICE - off);
// //     ok &= bqWrite(type, addr, start + off, buf + off, len);
// //   }
// //   return ok;
// // }

// // void bq_config_balancing() {
// //   bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL1, BAL_DUTY_CODE);
// //   bq_stop_balancing();
// // }

// // bool bq_start_balancing(const bool want[NUM_STACK_DEVICES][CELLS_PER_DEVICE]) {
// //   bool ok = true;
// //   for (int a = 1; a <= NUM_STACK_DEVICES; a++) {   // each board gets its own pattern
// //     uint8_t timers[CELLS_PER_DEVICE];
// //     for (int c = 0; c < CELLS_PER_DEVICE; c++) timers[c] = want[a - 1][c] ? CB_TIMER_CODE : 0;
// //     ok &= write_cb_timers((uint8_t)a, SINGLE_WRITE_VAL, timers);
// //   }
// //   // Auto mode: chip alternates odd/even cells, pauses on overtemp, stops on fault.
// //   ok &= bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL2,
// //                   BAL_CTRL2_AUTO_BAL | BAL_CTRL2_BAL_GO | BAL_CTRL2_OTCB_EN | BAL_CTRL2_FLTSTOP_EN);
// //   balancing = ok;
// //   return ok;
// // }

// // bool bq_stop_balancing() {
// //   uint8_t zeros[CELLS_PER_DEVICE] = {0};
// //   bool ok = write_cb_timers(0, STACK_WRITE_VAL, zeros);
// //   ok &= bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL2, BAL_CTRL2_BAL_GO);   // apply zero timers
// //   balancing = false;
// //   return ok;
// // }

// // bool bq_pause_balancing(bool pause) {
// //   uint8_t v = BAL_CTRL2_AUTO_BAL | BAL_CTRL2_OTCB_EN | BAL_CTRL2_FLTSTOP_EN;
// //   if (pause) v |= BAL_CTRL2_CB_PAUSE;
// //   return bqWrite1(STACK_WRITE_VAL, 0, REG_BAL_CTRL2, v);
// // }

// // bool bq_balancing_active() { return balancing; }