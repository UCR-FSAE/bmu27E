#include "bq_comm.h"

#define DIAG_PWR_CTRL 0x0337
#define FAULT_RST1 0x0331
#define FAULT_RST2 0x0332
#define FAULT_PWR1 0x0552
#define FAULT_PWR2 0x0553

// Power supply BIST -- Only checks PWR1 and PWR2 (PW3 not included)

struct PowerBistResult {
    uint8_t faultPwr1;
    uint8_t faultPwr2;
    bool pwrBistFail; // if 1 the BIST has failed
};

BmsStatus_t runPowerBist();
BmsStatus_t readPowerBistResult(PowerBistResult *result);
bool resetFaultRST1(uint8_t bits);

// call with 0x01 to reset RST_PWR
bool resetFaultRST1(uint8_t bits) {

    bool status = bqWrite(SINGLE_WRITE_VAL, 0, FAULT_RST1, &bits, 1);
    return status;
}

BmsStatus_t runPowerBist() {
    // reset all FAULT_PWR registers before doing anything
    if (!resetFaultRST1(0x01)) return BMS_ERR_FAULT; // probably not the right error

    uint8_t value = 0x03; // BIST_NO_RST = 1, PWR_BIST_GO = 1
    if (!bqWrite(SINGLE_WRITE_VAL, 0, DIAG_PWR_CTRL, &value, 1)) {
        return BMS_ERR_TIMEOUT; //check right error
    }

    return BMS_OK;
}

// TODO remember to wait at least 12.1 ms to let the BIST cycle finish (this is the max time)
BmsStatus_t readPowerBistResult(PowerBistResult *result) {
    if (result == nullptr) return BMS_ERR_FAULT; //to be replaced with invalid-argument status

    // first check if the operation was successful with  FAULT_PWR2[PWRBIST_FAIL]
    BmsStatus_t status = bqRead(SINGLE_READ_VAL, 0, FAULT_PWR2, 1, 1, &resp, 20);
    if (status != BMS_OK) return status;

    result->pwrBistFail = (resp.data[0][0] & 0x40); // the bit 6 must be 0 otherwise --> fault in BIST
    result->faultPwr2 = resp.data[0][0];

    // store fault_pwr1
    status = bqRead(SINGLE_READ_VAL, 0, FAULT_PWR1, 1, 1, &resp, 20);
    if (status != BMS_OK) return status;
    result->faultPwr1 = resp.data[0][0];

    return BMS_OK;
}
