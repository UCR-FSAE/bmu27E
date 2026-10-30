#include "temps.h"
#include <Arduino.h>
#include <stdint.h>

// ---=== BASIC PLAN
// get resistance using formula?: (GPIO_ADC / TSREF_ADC) = RNTC / (RNTC + R1)
// there's a built in voltage divider that lets us solve for resistance
// GPIO and TSREF values need to be converted to microvolts?

// evan doesnt think we need specific thermistor values? as in properties not read values
// if i need to know anything hardware related it will be on altium
// figure out how the old code works to see if applicable


// ---=== WHAT EACH VALUE REPRESENTS
// GPIO_ADC = raw ADC code from GPIO pin, node between R1 and thermistor. comes from GPIO*_HI/LO
// TSREF_ADC = raw ADC code from TSREF pin. from TSREF_HI/LO register
// RNTC = thermistor resisance im looking for
// R1 = known fixed pullup resistor between TSREF and GPIO. something we put in ourselves?
#define R1 = 10000.0f // BASED ON SOMETHING THAT PHYSICALLY EXISTS. CHANGE LATER?


// ---=== FROM THAT BULLSHIT AI FORMULA
// --- could use stein-something instead? but that needed 3 coefficients? compare to the old code
// all subject to change based on real thermistor values
#define BETA = 3950.0f // "constant from thermistor datasheet describing material's curve"
#define T0 = 298.15f // "reference temperature, always 25*C"
#define R25 = 10000.0f // "thermistor resistance at 25*C." obviously being replaced later

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


float getTemp(){
	//get resistance
	float RNTC = getResistance();

	//using a formula that's supposedly widely used? a beta formula? 
	// 1/T = 1/T0 + (1/β) × ln(RNTC / R25)
	float invTemp = 1/T0 + (1/BETA) * ln(RNTC/R25);
	float temp = 1/invTemp;

	return temp;
	
	retrurn 0;
}

// ---=== OLD THERMISTOR CODE
// --- he doesnt think we need to know the specifics of the component to get the temps? 
// --- but everything has changed since then so idk if this will work
// --- should try to decode this to see how this works? if it's still applicable?


// static float read_temperature() {
//   analogRead(ADC_PIN);
//   delayMicroseconds(20);

//   int32_t raw = 0;
//   for (int j = 0; j < 16; j++) raw += analogRead(ADC_PIN);
//   adcValue = raw / 16.0f;

//   if (adcValue <= 0 || adcValue >= ADC_MAX) return 0.0f;

//   float R = R_FIXED * adcValue / (ADC_MAX - adcValue);
//   float lnR = logf(R);
//   float inv_T = SH_A + SH_B * lnR + SH_C * lnR * lnR * lnR;
//   return 1.0f / inv_T - 273.15f;
// }