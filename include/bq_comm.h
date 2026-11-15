#pragma once

// BQ79600 / BQ79616 UART driver
// Software design ref (SDR): https://www.ti.com/lit/an/slvae86b/slvae86b.pdf
// Datasheet:                 https://www.ti.com/lit/gpn/bq79616-q1

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include "bq_constants.h"

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
// Replies stored by the device address inside each reply
// (0 = BQ79600, 1..N = BQ79616s).
typedef struct
{
    uint8_t data[BQ_MAX_DEVICES][BQ_MAX_RESP_BYTES];
    bool    valid[BQ_MAX_DEVICES];
    int     num_valid;
    int     crc_errors;
} BqResponse;

extern BqResponse resp;

typedef enum {
    BMS_OK = 0,
    BMS_ERR_TIMEOUT, // no response (device asleep, wiring, broken daisy chain)
    BMS_ERR_CRC,     // response came back but CRC bad -> retry
    BMS_ERR_INIT,    // wakeup / auto-address / config failed
    BMS_ERR_FAULT,   // device reports a real fault (OV/UV/OT etc.)
    BMS_ERR_PARAM    // sending read request, requesting too many or too little bytes
} BmsStatus_t;

typedef enum { READ_VOLT, READ_TEMP, READ_FAULT, READ_BRIDGE, READ_COUNT } Meas;

// ---------------------------------------------------------------------------
// CRC
// ---------------------------------------------------------------------------

// CRC-16 (datasheet p56). Running it over a complete, intact frame returns 0.
uint16_t crc_compute(const uint8_t *data, size_t length);

// ---------------------------------------------------------------------------
// Frame builders (return total frame length, 0 on error)
// ---------------------------------------------------------------------------
// data_len: 1-8 payload bytes. dev_addr only used for SINGLE_WRITE_VAL.
uint8_t build_write_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr,
                          uint16_t reg_addr, const uint8_t *data, uint8_t data_len);

// bytes_request: real byte count minus 1 (0x00 = 1 byte, 0x7F = 128 bytes).
// dev_addr only used for SINGLE_READ_VAL.
uint8_t build_read_frame(uint8_t *buf, uint8_t msgType, uint8_t dev_addr,
                         uint16_t reg_addr, uint8_t bytes_request);

// ---------------------------------------------------------------------------
// Low-level UART
// ---------------------------------------------------------------------------
void sendFrame(uint8_t *frame, uint8_t len);

// ---------------------------------------------------------------------------
// High-level read / write
// ---------------------------------------------------------------------------
// Collect up to expected_frames replies of nbytes data bytes each.
// Returns the number of valid (CRC-checked) replies.
BmsStatus_t bqReadResponse(uint8_t nbytes, int expected_frames, BqResponse *out, uint32_t timeout_ms);

// Write 1-8 bytes. msgType = SINGLE_/STACK_/BROAD_WRITE_VAL.
bool bqWrite(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint8_t data_len);
bool bqWrite1(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t value);

BmsStatus_t bqReadOnce(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t nbytes,
                       int expected_frames, BqResponse *out, uint32_t timeout_ms);
// Read nbytes (real count, 1-128) and collect replies.
// msgType = SINGLE_/STACK_/BROAD_READ_VAL.
BmsStatus_t bqRead(uint8_t msgType, uint8_t dev_addr, uint16_t reg_addr, uint8_t nbytes, int expected_frames, BqResponse *out, uint32_t timeout_ms);

// ---------------------------------------------------------------------------
// BQ init functions
// ---------------------------------------------------------------------------
void bq_wake();
void bqUartInit(); // Call once in setup(). Enlarges the RX buffer for stack reads.
void wakePing();   // wake ping
void bqFlushRx();  // Discard any stale bytes before a new read.
bool bq_auto_address();

// ---------------------------------------------------------------------------
// Application-level
// ---------------------------------------------------------------------------
void readVoltages();
void readTemps();
void readFaultFlags();
void rebalanceVoltages();

// ---------------------------------------------------------------------------
// Message checking
// ---------------------------------------------------------------------------
void markOk(Meas m);
void resetAll();
bool commLost();
// ---------------------------------------------------------------------------ù
// Faults
// ---------------------------------------------------------------------------
typedef enum
{
    FAULT_PWR,
    FAULT_SYS,
    FAULT_OVUV,
    FAULT_OTUT,
    FAULT_COMM,
    FAULT_OTP,
    FAULT_COMP_ADC,
    FAULT_PROT,
} FaultGroup;

typedef struct
{
    bool protection; //2
    bool comparatorADC; //10
    bool otp; //1
    bool communication; //3
    bool overTempUnderTemp; //2
    bool overVoltageUnderVoltage; //4
    bool system; //1
    bool power; //3
} FaultSummary;

typedef struct
{
    bool communication;
    bool registers;
    bool system;
    bool power;
} BridgeFaultSummary;

//check which faults the bridge has 
//index 0 is not used since it belongs to the bridge
typedef struct {
    uint8_t summary[13]; //fault summary
    uint8_t comm1[13];
    uint8_t comm2[13];
    uint8_t comm3[13];
    uint8_t otp[13];
    uint8_t sys[13];
    uint8_t prot1[13];
    uint8_t prot2[13];
    uint8_t ov1[13];
    uint8_t ov2[13];
    uint8_t uv1[13];
    uint8_t uv2[13];
    uint8_t ot[13];
    uint8_t ut[13];
    uint8_t comp_gpio[13];
    uint8_t comp_vccb1[13];
    uint8_t comp_vccb2[13];
    uint8_t comp_vcow1[13];
    uint8_t comp_vcow2[13];
    uint8_t comp_cbow1[13];
    uint8_t comp_cbow2[13];
    uint8_t comp_cbfet1[13];
    uint8_t comp_cbfet2[13];
    uint8_t comp_misc[13];
    uint8_t pwr1[13];
    uint8_t pwr2[13];
    uint8_t pwr3[13];
}FaultRegisters; 

typedef struct 
{
    uint8_t reg;
    uint8_t sys;
    uint8_t pwr;
    uint8_t comm1;
    uint8_t comm2;
}BridgeFaultRegisters;


FaultSummary decodeFaultSummary(uint8_t summary); //decodes the 8 bits summary into a boolean struct
BridgeFaultSummary decodeBridgeFaultSummary(uint8_t summary);
BmsStatus_t readFaultSummary(); //reads summaries from all 12 BQ
BmsStatus_t readBridgeFaultSummary(); //reads bridge's summary
BmsStatus_t handleFaultSummary(FaultSummary *summary, FaultRegisters *registers); //in case a fault is true reads through the corresponding registers  
BmsStatus_t handleBridgeFaultSummary(BridgeFaultSummary *summary, BridgeFaultRegisters *registers);   
