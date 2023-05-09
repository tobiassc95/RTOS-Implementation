/*
 * encoder.h
 *
 *  Created on: 2 sep. 2019
 *      Author: guido
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdbool.h>
#include  <os.h>

typedef enum {CLOCKWISE, ANTCLOCKWISE} twist_dir_t;	// Sentidos en los que se gire el encoder
typedef enum {DOWN, UP} switch_state_t;				// Estados del switch
typedef enum {CLOCKWISE_EV, ANTCLOCKWISE_EV, DOWN_EV, UP_EV} enc_event_t;	// Eventos posibles del encoder

void init_encoder(void);							// funcion que inicializa el encoder

void encoder_enable(void);							// Habilita el encoder

bool check_new_twist(void);							// check si la perilla fue girada

bool check_new_switch(void);						// check si el switch fue pulsado

twist_dir_t get_last_twist(void);					// Ultimo sentido en le que fue girada la perilla

switch_state_t get_last_switch(void);				// Ultimo estado en el que estuvo el switch

void encoder_disable(void);							// Deshabilita el encoder

void enc_set_queue(OS_Q* osqueue);

void enc_set_sem(OS_SEM* sem_enc);

#endif /* ENCODER_H_ */
