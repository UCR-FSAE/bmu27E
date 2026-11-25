#include "bq_comm.h"

#define DIAG_PWR_CTRL 0x0337
#define FAULT_RST1 0x0331
#define FAULT_RST2 0x0332
#define FAULT_PWR1 0x0552
#define FAULT_PWR2 0x0553

//Power supply BIST -- Only checks PWR1 and PWR2 (PW2 not included)

struct PowerBistResult {
    uint8_t faultPwr1;
    uint8_t faultPwr2;
};

BmsStatus_t runPowerBist();
BmsStatus_t readPowerBistResult(PowerBistResult *result);
bool resetRegisters();

bool resetRegisters(){
    uint8_t value = 0xFF;
    bool status = bqWrite(SINGLE_WRITE_VAL, 0 ,FAULT_RST1, &value, 1);
    if(!status){} //manage error
    status = bqWrite(SINGLE_WRITE_VAL, 0 ,FAULT_RST2, &value, 1);
    if(!status){} //manage error
    return status;
}

BmsStatus_t runPowerBist() {
    //reset all fault registers before doing anything
    resetRegisters();

    uint8_t value = 0x03; // BIST_NO_RST = 1, PWR_BIST_GO = 1
    bool status = bqWrite(SINGLE_WRITE_VAL, 0 ,DIAG_PWR_CTRL , &value, 1); 
    if(!status){} //manage error

    return BMS_OK;
}


//TODO remember to wait at least 12.1 ms to let the BIST cycle finish (this is the max time)
BmsStatus_t readPowerBistResult(PowerBistResult *result){
    //first check if the operation was successful with  FAULT_PWR2[PWRBIST_FAIL]
    BmsStatus_t status = bqRead(SINGLE_READ_VAL, 0, FAULT_PWR2, 1, 1, &resp, 20);
    if(status != BMS_OK) {} //menage error

    if((resp.data[0][0] & 0x80)) return BMS_ERR_FAULT; //the last bit must be 0 otherwise --> fault in BIST
    result->faultPwr2 = resp.data[0][0];

    //store fault_pwr1 and fault_pwr2 (already stored)
    status = bqRead(SINGLE_READ_VAL, 0, FAULT_PWR1, 1, 1, &resp, 20);
    if(status != BMS_OK) {} //menage error
    result->faultPwr1 = resp.data[0][0];

    return status;
}



