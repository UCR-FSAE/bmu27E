// hi
#ifndef TEMPS_H
#define TEMPS_H

#include <stdint.h>

int16_t getTemp(uint16_t deviceID, uint8_t GPIOHIAddr);

// main file currently using read/write from example code. probably wont work bc its not UART 
// bq79616.cpp has part of a read func. but not complete. plan to replace when done
// SpiReadReg(<cmu ID>, <register address>, <buffer>, <# of bytes>, <max waiting time>, <read type>)

#endif