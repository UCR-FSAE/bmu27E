// hi
#ifndef TEMPS_H
#define TEMPS_H

#include <stdint.h>

// ---=== STEINHART NUMBERS
// STEIN_<A-C> = based on thermistor material/manufacturing process. likely different for the current thermistor?
// R_CONST = known fixed pullup resistor between TSREF and GPIO. old therm said 10k ohms, might be different now?
#define STEIN_A 1.1395e-3f
#define STEIN_B 2.3230e-4f
#define STEIN_C 9.5816e-8f
#define R_CONST 10000.0f 	// BASED ON SOMETHING THAT PHYSICALLY EXISTS. CHANGE LATER? 

void readTemps();


#endif