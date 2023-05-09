#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

//configura todo el display.
void displayConfiguration (void);
//ponemos el valor que queremos escribir al display.
void displayWriteDig (uint8_t dig, uint8_t value);
void displayWriteChar (uint8_t dig, char value);
void displayWriteNumb (uint16_t value);
void displayWriteStr (char* value);
void displayWriteDot (uint8_t dig);
void displayCleanDig (uint8_t dig);
void displayCleanDot (uint8_t dig);
void displayCleanAll (void);

#endif //DISPLAY_H
