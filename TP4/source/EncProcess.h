/*
 * encProcess.h
 *
 *  Created on: 11 sep. 2019
 *      Author: guido
 */

#ifndef ENCPROCESS_H_
#define ENCPROCESS_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {ID_READY, ID_NOT_READY, ID_CANCELLED} ID_state_t;
typedef enum {PIN_READY, PIN_NOT_READY, PIN_CANCELLED} PIN_state_t;

void update_ID(void);
ID_state_t get_ID_state(void);
bool get_ID_value(uint8_t* ID_array);

void update_PIN(void);
PIN_state_t get_PIN_state(void);
bool get_PIN_value(uint8_t* PIN_array);

void restart_ID(void);
void restart_PIN(void);

void ID_cancel_request(void);
void PIN_cancel_request(void);

void restart_ID_state(void);
void restart_PIN_state(void);

#endif /* ENCPROCESS_H_ */
