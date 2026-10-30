#include "temps.h"
#include <Arduino.h>
#include <stdint.h>

//get resistance using formula?: (GPIO_ADC / TSREF_ADC) = RNTC / (RNTC + R1)
// there's a built in voltage divider that lets us solve for resistance
// GPIO and TSREF values need to be converted to microvolts?

// GPIO_ADC = raw ADC code from GPIO pin, node between R1 and thermistor. comes from GPIO*_HI/LO
// TSREF_ADC = raw ADC code from TSREF pin. from TSREF_HI/LO register
// RNTC = thermistor resisance im looking for
// R1 = known fixed pullup resistor between TSREF and GPIO. something we put in ourselves?
#define R1 = 10000 // SUBJECT TO CHANGE

// all subject to change based on real thermistor values
#define BETA = 3950 // "constant from thermistor datasheet describing material's curve"
#define T0 = 298.15 // "reference temperature, always 25*C"
#define R25 = 10000 // "thermistor resistance at 25*C." obviously being replaced later

//get voltage & calculate based on that?
//helps readTemps
static float getResistance(){
	//read gpio & tsref voltages from bq79616 registers
	// i saw spi read/writes in the sample code can we transfer those here?

	//convert both values to microvolts for resistance formula

	//calulate resistance
	float res = R1 * GPIO_ADC / (TSREF_ADC - GPIO_ADC);

	//return value
	return res;
}


int16_t getTemp(){
	//get resistance
	float RNTC = getResistance();

	//using a formula that's supposedly widely used? a beta formula? 
	// 1/T = 1/T0 + (1/β) × ln(RNTC / R25)
	int16_t invTemp = 1/T0 + (1/BETA) * ln(RNTC/R25);
	int16_t temp = 1/invTemp;

	return temp;
	
	retrurn 0;
}

