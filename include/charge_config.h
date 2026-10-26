// charge_config.h — constants for wake, auto-addressing, balancing and charging.
// TODO values come from the cell datasheet / final pack design.
#pragma once
#include <stdint.h>

// ============================== Pack layout ==============================
#define NUM_STACK_DEVICES     6     // number of BQ79616 boards. TODO: 6 or 12?
#define CELLS_PER_DEVICE      10
#define TOTAL_CELLS           (NUM_STACK_DEVICES * CELLS_PER_DEVICE)
#define TOTAL_DEVICES         (NUM_STACK_DEVICES + 1)   // +1 = BQ79600 bridge (address 0)

// ============================== Balancing ==============================
#define BAL_THRESHOLD_MV          15     // balance cells this far above the lowest cell
#define BAL_MIN_CELL_MV           3900   // only balance near the top of charge
#define BAL_REEVALUATE_MS         30000  // recompute which cells to balance

// ============================== Charging ==============================
#define CELL_CHARGE_TARGET_MV     4150   // TODO: chemistry dependent
#define CELL_TAPER_START_MV       4100   // start reducing current here
#define CELL_CHARGE_I_MAX_A       10.0f  // TODO: cell max charge current x parallel count
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
#define BQ_UART_BAUD          1000000
#define CAN_CAR_BAUD          500000
#define CAN_CHARGER_BAUD      250000
#define PIN_BQ_TX             43   // -> BQ79600 RX   TODO: confirm with schematic
#define PIN_BQ_RX             44   // <- BQ79600 TX
#define CAN_TX_GPIO           GPIO_NUM_5   // same pins as ESP32_CAN.cpp
#define CAN_RX_GPIO           GPIO_NUM_4