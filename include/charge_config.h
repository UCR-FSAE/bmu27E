// charge_config.h — constants for wake, auto-addressing, balancing and charging.
// values from the cell datasheet / final pack design.
#pragma once
#include <stdint.h>

// ============================== Pack layout ==============================
// 6 subpacks x 60 cells = 360 cells, monitored by 12 BQ79616s (2 per subpack).
// 20s3p per subpack -> 120s3p pack: each BQ79616 sees 10 series groups,
#define NUM_STACK_DEVICES     12    // number of boards 
#define CELLS_PER_DEVICE      10    // series
#define CELLS_PARALLEL        3     // cells in parallel per series group
#define TOTAL_CELLS           (NUM_STACK_DEVICES * CELLS_PER_DEVICE)   // series groups 
#define TOTAL_DEVICES         (NUM_STACK_DEVICES + 1)   

// ============================== Balancing ==============================
// all in millivolts
// ms in millisecs
#define BALANCING_THRESHOLD_MV          15     // 15mv to  balance cells
#define BALANCING_MIN_CELL_MV           3900   // cell balanced if at least 3.9v
#define BALANCING_REEVALUATE_MS         30000  // rededcides cell balancing every 30 secs

// ============================== Charging ==============================
// in millivolts

#define CELL_CHARGE_TARGET_MV     4150   // 4.15v to stop at (per cell group)
#define CELL_TAPER_START_MV       4100   // when current reduces (4.1v)
#define CELL_CHARGE_I_MAX_A       (3.3f * CELLS_PARALLEL)  // TODO: cell max charge current x parallel count
#define CHARGE_END_CURRENT_A      1.0f   // done when current tapers below this
#define CHARGE_RAMP_A_PER_S       2.0f   // max rise rate of the current request
#define CHARGE_TEMP_MIN_C         0.0f   // no charging below this
#define CHARGE_TEMP_MAX_C         45.0f  // no charging above this
#define CHARGE_DERATE_START_C     40.0f  // linear derate from here to CHARGE_TEMP_MAX_C
#define CHARGE_OVERCURRENT_MARGIN_A 3.0f // measured > requested + this => abort

// ============================== Elcon HK-LF-312-20 ==============================
#define CHARGER_V_MAX             450.0f
#define CHARGER_V_MIN             200.0f
#define CHARGER_I_MAX             20.0f
#define CHARGER_P_MAX_W           6600.0f   // 3300 W on 110 VAC
#define ELCON_CMD_ID              0x1806E5F4
#define ELCON_STATUS_ID           0x18FF50E5
#define ELCON_CMD_PERIOD_MS       500       // charger times out at ~5 s
#define ELCON_SILENT_MS           2000      // our stricter timeout
#define ELCON_DETECT_TIMEOUT_MS   3000      // wait this long for the first status frame

// ============================== Comms / pins (XIAO ESP32-S3) ==============================
// #define BQ_UART_BAUD          1000000   // now set in bq79616.cpp
// #define CAN_CAR_BAUD          500000
// #define CAN_CHARGER_BAUD      250000
// #define PIN_BQ_TX             43        // now TX_PIN in bq79616.cpp
// #define PIN_BQ_RX             44        // now RX_PIN in bq79616.cpp
#define CAN_TX_GPIO           GPIO_NUM_5   // must match ESP32_CAN.cpp (pinout sheet says GPIO 41)
#define CAN_RX_GPIO           GPIO_NUM_4   // must match ESP32_CAN.cpp (pinout sheet says GPIO 39)

// ^^ still need to confirm w schematics