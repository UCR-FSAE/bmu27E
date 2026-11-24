#include <Arduino.h>
#include <stdint.h>
#include <math.h>

#include "temps.h"
#include "bq_comm.h"

/* ---=== COMPUTE TEMP
helper function for readTemps()
converts register value to valid measurement of microvolts
uses two voltages to calculate resistance
uses resistance to calculate temperature in kelvin using steinhart formula
shift temp to return as celcius
*/
static float computeTemp(uint16_t gpio_raw, uint16_t tsref_raw){
	// gpio and tsref converted to microvolts
	// 2's compliment multiplied by scaling value from bq79616 datasheet
    float gpio_uV  = (float)(int16_t)gpio_raw * 152.59f;	// 152.59 is for GPIO*_HI/LO
    float tsref_uV = (float)(int16_t)tsref_raw * 169.54f; 	// 169.54 is for TSREF_HI/LO

	if (gpio_uV <= 0.0f || gpio_uV >= tsref_uV) return NAN;// old thermistor code considered gpio_V <= 0 || >= tsref_V as errors.
	
	float res = R_CONST * gpio_uV / (tsref_uV - gpio_uV); 	// calculate resistance

	// use steinhart to calculate temperature
	float ln_res = logf(res);
	float invTemp = STEIN_A + STEIN_B * ln_res + STEIN_C * ln_res * ln_res * ln_res; 	// 1/T = A + B·ln(R) + C·(ln(R))³
	float temp = 1.0f / invTemp - 273.15f; 												// - 273 to get temps in C. Steinhart gives Kelvin.

	return temp;
}


/* ---=== GET TEMP : STACK READ
get readings from BqResponse.
	- read stack for 12 GPIOs & store raw data in array
	- read stack for 12 TSREFs & store raw data in array
	
then calculate temps and update 
*/
void readTemps(){
	//data from bq
	BqResponse response;

	// raw values from bq pins
	uint16_t gpio_raw[BQ_MAX_DEVICES];
	bool gpio_valid[BQ_MAX_DEVICES];

	uint16_t tsref_raw[BQ_MAX_DEVICES];
	bool tsref_valid[BQ_MAX_DEVICES];

 	// read from gpio and tsref registers & markOk. Ignore 1st decvice (master)
	if(bqRead(STACK_READ_VAL, 0, 0x058E, 2, NUM_STACK_DEVICES, &response, 100) != BMS_OK){return;}
	for(int i = 1; i <= NUM_STACK_DEVICES; i++){
		gpio_valid[i] = response.valid[i];
		gpio_raw[i] = (uint16_t)(response.data[i][0] << 8) | response.data[i][1];
	}
	
	if(bqRead(STACK_READ_VAL, 0, 0x058C, 2, NUM_STACK_DEVICES, &response, 100) != BMS_OK){return;}
	for(int i = 1; i <= NUM_STACK_DEVICES; i++){
		tsref_valid[i] = response.valid[i];
		tsref_raw[i] = (uint16_t)(response.data[i][0] << 8) | response.data[i][1];
	}

	measOk(READ_TEMP);

	// compute each temp individually and update cell_temps 
	for(int i = 1; i < NUM_STACK_DEVICES; i++){
		if(gpio_valid[i] && tsref_valid[i]){
			cell_temps[i] = computeTemp(gpio_raw[i], tsref_raw[i]);
		}
	}
}
