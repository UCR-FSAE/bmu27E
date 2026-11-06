#include "temps.h"
#include <Arduino.h>
#include <stdint.h>
#include <math.h>

// ---=== BASIC PLAN
// get resistance using formula: res = R_CONST * gpio_uv / (tsref_uv - gpio_uv);
// old thermistor code uses steinhart formula w/ the 3 coefficients A, B, C.
// find out what current thermistor values are from altium
// evan needs to meet w/ other teams during scope meeting to figure this out but assume old stuff still being used for now


// ---=== STEINHART NUMBERS
// STEIN_<A-C> = based on thermistor material/manufacturing process. likely different for the current thermistor?
// R_CONST = known fixed pullup resistor between TSREF and GPIO. old therm said 10k ohms, might be different now?
#define STEIN_A 1.1395e-3f
#define STEIN_B 2.3230e-4f
#define STEIN_C 9.5816e-8f
#define R_CONST 10000.0f // BASED ON SOMETHING THAT PHYSICALLY EXISTS. CHANGE LATER? 


float getTemp(uint16_t deviceID){
	// need buffer b/c spi read gives one byte at a time
	uint8_t buffer[2];

	// raw values from bq pins
	uint16_t gpio_raw = 0;
	uint16_t tsref_raw = 0;

 	// read from gpio and tsref registers
	// SpitReadReg from example code is placeholder for actual register read func.
	SpiReadReg(deviceID, 0x058E, buf, 2, 100, 0x00);	// GPIO hi/lo. does single device read.
    gpio_raw = (buffer[0] << 8) | buffer[1];

    SpiReadReg(deviceID, 0x058C, buf, 2, 100, 0x00); 	// TSREF hi/lo
    tsref_raw = (buffer[0] << 8) | buffer[1];

	// gpio and tsref converted to microvolts
	// 2's compliment multiplied by scaling value from bq79616 datasheet
    float gpio_uV  = (float)(int16_t)gpio_raw * 152.59f;	// 152.59 is for GPIO*_HI/LO
    float tsref_uV = (float)(int16_t)tsref_raw * 169.54f; 	// 169.54 is for TSREF_HI/LO

	if (gpio_uV <= 0.0f || gpio_uV >= tsref_uV) return 0.0f;// old thermistor code considered gpio_V <= 0 || >= tsref_V as errors. returned 0.
	
	float res = R_CONST * gpio_uV / (tsref_uV - gpio_uV); 	// calculate resistance

	// use steinhart to calculate temperature
	float ln_res = logf(res);
	float invTemp = STEIN_A + STEIN_B * ln_res + STEIN_C * ln_res * ln_res * ln_res; 	// 1/T = A + B·ln(R) + C·(ln(R))³
	float temp = 1.0f / invTemp - 273.15f; 												// - 273 to get temps in C. Steinhart gives Kelvin.

	return temp;
}