/*
    Constants
*/

#define EVM_TEST

#pragma once
// ---------------------------------------------------------------------------
// Peripheral constants
// ---------------------------------------------------------------------------
#define UART_SERIAL Serial1
#define RX_PIN D7
#define TX_PIN D6

// ---------------------------------------------------------------------------
// bq init/comm constants
// ---------------------------------------------------------------------------
#define BQ_UART_BAUD       1000000   // BQ79600 UART speed
#define BQ_MAX_DEVICES     13        // BQ79600 (addr 0) + 12 x BQ79616 (addr 1..12)
#define BQ_MAX_RESP_BYTES  128       // largest read the chips allow

#ifdef EVM_TEST

#define TOTAL_DEVICES      2
#define NUM_STACK_DEVICES  1

#define BQ_MAX_RETRIES  3        // re-requests after the first attempt

#else

#define TOTAL_DEVICES      13
#define NUM_STACK_DEVICES  12

#endif

// ---------------------------------------------------------------------------
// Message type definitions (command byte upper nibble)
// ---------------------------------------------------------------------------
// Single device
#define SINGLE_READ_VAL     0x80
#define SINGLE_WRITE_VAL    0x90
// Stack
#define STACK_READ_VAL      0xA0
#define STACK_WRITE_VAL     0xB0
// Broadcast
#define BROAD_READ_VAL      0xC0
#define BROAD_WRITE_VAL     0xD0
#define BROAD_WRITE_REVERSE 0xE0

// ---------------------------------------------------------------------------
// registers used for wake, auto-addressing and balancing
// ---------------------------------------------------------------------------

// ---------------- Wake / auto-addressing ----------------
#define REG_DIR0_ADDR          0x0306
#define REG_COMM_CTRL          0x0308   // bit1 STACK_DEV, bit0 TOP_STACK
#define REG_CONTROL1           0x0309   // bit0 ADDR_WR, bit5 SEND_WAKE (600)
#define REG_OTP_ECC_DATAIN1    0x0343   // ECC_DATA1..8: DLL sync dummy write/read

#define COMM_CTRL_BASE         0x00
#define COMM_CTRL_STACK_DEV    0x02
#define COMM_CTRL_TOP_STACK    0x01
#define CONTROL1_ADDR_WR       0x01
#define CONTROL1_SEND_WAKE     0x20

// Clearing faults after DLL sync (Table 7-19 step 8)
#define REG_FAULT_RST1         0x0331   // 616 (FAULT_RST2 follows at 0x0332)
#define REG_FAULT_RST_600      0x2030   // 600

// ---------------- Balancing (616, auto mode) ----------------
#define REG_CB_CELL1_CTRL      0x0327   // CB_CELL16_CTRL (0x0318) ... CB_CELL1_CTRL (0x0327)
#define REG_BAL_CTRL1          0x032E   // duty cycle
#define REG_BAL_CTRL2          0x032F
#define REG_DEV_CONF1_600        0x2001
#define DEV_CONF1_600_DEFAULT    0x14
#define BAL_CTRL2_AUTO_BAL     0x01
#define BAL_CTRL2_BAL_GO       0x02
#define BAL_CTRL2_OTCB_EN      0x10     // pause balancing on overtemp
#define BAL_CTRL2_FLTSTOP_EN   0x20     // stop balancing on fault
#define BAL_CTRL2_CB_PAUSE     0x40     // pause (for clean voltage readings)
#define BAL_DUTY_CODE          0x01     // TODO: from BAL_CTRL1 table
#define CB_TIMER_CODE          0x02     // TODO: from CB_CELLx_CTRL table; longer than BAL_REEVALUATE_MS

// ---------------- Timing ----------------
#define WAKE_PING_US           2750      // wake ping low time
#define WAKE_SETTLE_MS         5
#define STACK_WAKE_PER_DEV_MS  10     // 616 from SHUTDOWN: up to 10 ms per device
#define STACK_WAKE_MS          (NUM_STACK_DEVICES * STACK_WAKE_PER_DEV_MS + 5)