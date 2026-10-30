// hi
#ifndef TEMPS_H
#define TEMPS_H

#include <stdint.h>

//resistance decrease when temps increase?

//old plan:
// read voltage -> determine resistance -> determine temps -> return

// dont need a different read voltage bc voltage divider built into chip, so i can get resistance directly

int16_t getTemp();

#endif