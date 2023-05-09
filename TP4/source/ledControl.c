/*
 * ledControl.c
 *
 *  Created on: 11 sep. 2019
 *      Author: guido
 */

#include "ledControl.h"
#include "portpin.h"

#define PIN_STATUS_0	PORTNUM2PIN(PORT_E,24) // PTB22
#define PIN_STATUS_1	PORTNUM2PIN(PORT_E,25) // PTE26

void led_init(void)
{
	PINconfigure(PIN_STATUS_0, PIN_MUX1, PIN_IRQ_DISABLE);
	PINconfigure(PIN_STATUS_1, PIN_MUX1, PIN_IRQ_DISABLE);
//	pinGPIOconfiguration(PIN_STATUS_0);
//	pinGPIOconfiguration(PIN_STATUS_1);
	PINpull(PIN_STATUS_0, PIN_PULLDOWN);
	PINpull(PIN_STATUS_1, PIN_PULLDOWN);
	PINmode(PIN_STATUS_0, PIN_OUTPUT);							// Vamos a usar los pines 24 y 25 del puerto E
	PINmode(PIN_STATUS_1, PIN_OUTPUT);
	PINwrite(PIN_STATUS_0, LOW);						// Estando ambos en bajo el Y0 estaria en alto
	PINwrite(PIN_STATUS_1, LOW);						//  que es la salida que no va a ningun led
}

void D1_ON(void)
{
	PINwrite(PIN_STATUS_0, HIGH);
	PINwrite(PIN_STATUS_1, LOW);
}

void D2_ON(void)
{
	PINwrite(PIN_STATUS_0, LOW);
	PINwrite(PIN_STATUS_1, HIGH);
}

void D3_ON(void)
{
	PINwrite(PIN_STATUS_0, HIGH);
	PINwrite(PIN_STATUS_1, HIGH);
}

void leds_OFF(void)
{
	PINwrite(PIN_STATUS_0, LOW);
	PINwrite(PIN_STATUS_1, LOW);
}
