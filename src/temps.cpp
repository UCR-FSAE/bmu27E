#include <Arduino.h>
#include <stdint.h>
#include <math.h>

#include "bq_comm.h"

/* ---=== STEINHART NUMBERS
STEIN_<A-C> = based on thermistor material/manufacturing process. likely different for the current thermistor?
R_CONST = known fixed pullup resistor between TSREF and GPIO. old therm said 10k ohms, might be different now?
*/
#define STEIN_A 1.1395e-3f
#define STEIN_B 2.3230e-4f
#define STEIN_C 9.5816e-8f
#define R_CONST 10000.0f 	// BASED ON SOMETHING THAT PHYSICALLY EXISTS. CHANGE LATER? 

// ---=== RULES-RELATED VALUES
// TODO: figure out how we deal w/ bad reads
// TODO: find the max value for the cell's datasheet
#define TEMP_MAX_C 60.0f	// rules say temps cant be below 60*C of the cell's max on the datasheet
// temp_overheat = false;			// tracks if any overheats exist
// temp_missing = false;			// tracks if bad/missing reads exist

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

if a cell fails to read, return early
TODO: figure out the proper response to failed reads
*/
void readTemps(){
	//data from bq
	BqResponse response;

	// raw values from bq pins
	// has 13 total, 12 cells + master board
	uint16_t gpio_raw[BQ_MAX_DEVICES]; 
	bool gpio_valid[BQ_MAX_DEVICES];

	uint16_t tsref_raw[BQ_MAX_DEVICES];
	bool tsref_valid[BQ_MAX_DEVICES];

	// clear cell_temps of old values
	for(int i = 0; i < NUM_STACK_DEVICES; i++){
		cell_temps[i] = NAN;
	}

	// clear bad read trackers
	temp_overheat = false;
	temp_missing = false;

 	// read from gpio and tsref registers & markOk. Ignore 1st decvice (master)
	if(bqRead(STACK_READ_VAL, 0, 0x058E, 2, NUM_STACK_DEVICES, &response, 100) != BMS_OK){temp_missing = true; return;}
	for(int i = 1; i <= NUM_STACK_DEVICES; i++){
		gpio_valid[i] = response.valid[i];
		gpio_raw[i] = (uint16_t)((response.data[i][0] << 8) | response.data[i][1]);
	}
	
	if(bqRead(STACK_READ_VAL, 0, 0x058C, 2, NUM_STACK_DEVICES, &response, 100) != BMS_OK){temp_missing = true; return;}
	for(int i = 1; i <= NUM_STACK_DEVICES; i++){
		tsref_valid[i] = response.valid[i];
		tsref_raw[i] = (uint16_t)((response.data[i][0] << 8) | response.data[i][1]);
	}

	// compute each temp individually and update cell_temps 
	for(int i = 1; i <= NUM_STACK_DEVICES; i++){
		if(gpio_valid[i] && tsref_valid[i]){
			cell_temps[i-1] = computeTemp(gpio_raw[i], tsref_raw[i]);
		}

		// check for bad reads (per rules)
		if(isnan(cell_temps[i-1])){temp_missing = true;}
		else if(cell_temps[i-1] > TEMP_MAX_C){temp_overheat = true;}
	}

	if(!temp_missing){measOk(READ_TEMP);}
}
