/*
 * encProcess.c
 *
 *  Created on: 11 sep. 2019
 *      Author: guido
 */


#include "encProcess.h"
#include "portpin.h"
#include "encoder.h"
#include "hardware.h"
#include "display.h"

#define ID_TOTAL_NUMBERS 8
#define PIN_TOTAL_NUMBERS 4
#define NOT_MODIFY_MOD	0
#define TOTAL_DISPLAYS	4

static ID_state_t ID_state = ID_NOT_READY;
static uint8_t ID[ID_TOTAL_NUMBERS];
static PIN_state_t PIN_state = PIN_NOT_READY;
static uint8_t PIN[PIN_TOTAL_NUMBERS];

static uint8_t display_pos  = 0;
static bool switch_st = 1;						// Valor del switch, se inicializa en 1 que es lo que tiene cuadno no se pulsa
static uint8_t actua_value = 0;


void update_ID(void)
{
	//static uint8_t disp_modif_pos  = 0;				// Uno de los 4 displays que se utilizan
	static bool selecting_data = false;
	static bool data2modSelected = false;
	static bool twist_made = false;						// Si hubo un nuevo giro
	static twist_dir_t twist_dir;
	static bool cancel_req_on = false;

	if(check_new_switch())						// Si hubo una pulsacion
	{
		if(get_last_switch() == UP)				// Cuando se levanto el pulsador puedo querer pasa el dato, moverme a uno que quiero modificar o decirle que ya lo modifique
		{
			switch_st = 1;
			if(cancel_req_on)
			{
				//TIMERstop(2);
				cancel_req_on = false;
			}
			if(selecting_data)
			{
				selecting_data = false;
				data2modSelected = true;
			}
//			else if(data2modSelected)
//			{
//				ID[disp_modif_pos] = actua_value;
//				data2modSelected = false;
//				actua_value = 0;
//				disp_modif_pos = display_pos;
//			}
			else
			{
				if(data2modSelected == true)
					data2modSelected = false;
				ID[display_pos] = actua_value;
				actua_value = 0;
				display_pos++;
				//disp_modif_pos = display_pos;
			}
		}
		else if(get_last_switch() == DOWN)				// Cuando se levanto el pulsador puedo querer pasa el dato, moverme a uno que quiero modificar o decirle que ya lo modifique
		{
			switch_st = 0;
			cancel_req_on = true;
			//TIMERloadISR(2, ID_cancel_request, 2500, TIMER_ISRONCE);		// Se inicia el timer para la cancelacion
//			TIMERenable();
			//TIMERstart(2);
		}
	}

	if(check_new_twist())
	{
		if(cancel_req_on)								// Se detiene el timer para la cancelacion
		{
			//TIMERstop(2);
			cancel_req_on = false;
		}

		twist_made = true;
		if(get_last_twist() == CLOCKWISE)		// Incremento o decremento segun corresponda
			twist_dir = CLOCKWISE;
		else if(get_last_twist() == ANTCLOCKWISE)		// Si es menor a cero no lo vas a queres decrementar
			twist_dir = ANTCLOCKWISE;
	}

	if(twist_made)							// Si hubo un nuevo giro
	{
		if(switch_st == 1)					// Si el switch esta arriba
		{
			if((twist_dir == CLOCKWISE) && (actua_value < 9))
				actua_value++;
			else if(twist_dir == ANTCLOCKWISE && (actua_value > 0))
				actua_value--;
		}
		else								// SI el switch esta abajo
		{
			selecting_data = true;
			if(twist_dir == ANTCLOCKWISE && (display_pos > 0)) {
				display_pos--;
				//display_pos = disp_modif_pos + 1;
			}
		}
		twist_made = false;
	}




	if(display_pos == ID_TOTAL_NUMBERS)			// Si ya se ingresaron todos los digitos se devuelve TRUE
	{
		actua_value = 0;
		display_pos = 0;
		ID_state = ID_READY;
		//TIMERstop(2);
		cancel_req_on = false;

	}
	else if(!data2modSelected && !selecting_data)		// Modo de diplay cuando no se esta haciendo una modificacion
	{
		switch(display_pos)
		{
		case 0:
			displayCleanDig(0);
			displayCleanDig(1);
			displayCleanDig(2);
			displayWriteDig(3,actua_value);
			break;
		case 1:
			displayCleanDig(0);
			displayCleanDig(1);
			displayWriteDig(2, ID[display_pos - 1]);
			displayWriteDig(3, actua_value);
			break;
		case 2:
			displayCleanDig(0);
			displayWriteDig(1, ID[display_pos - 2]);
			displayWriteDig(2, ID[display_pos - 1]);
			displayWriteDig(3, actua_value);
			break;
		default:
			displayWriteDig(0, ID[display_pos - 3]);
			displayWriteDig(1, ID[display_pos - 2]);
			displayWriteDig(2, ID[display_pos - 1]);
			displayWriteDig(3, actua_value);
			break;
		}
	}
	else		// Modo de diplay cuando se esta haciendo una modificacion
	{
		if(display_pos < 4) {
			int i;
			for(i = 0; i < 3; i++)
			{
				if(display_pos - (3 - i) >= 0)
					displayWriteDig(i,ID[display_pos - (3 - i)]);
				else
					displayCleanDig(i);
			}
		}
		else {
			int i;
			for(i = 0; i < 3; i++)
			{
				displayWriteDig(i,ID[display_pos - (3 - i)]);
			}
		}
		displayWriteDig(3,actua_value);
		//display_pos = disp_modif_pos; //a medida que nos desplazamos, se van borrando los numeros.
		//
//		if(display_pos < 4) //si todavia no se escribio mas de 4 numeros...
//		{
//			int i;
//			for(i = 0; i < display_pos; i++)
//			{
//				if((3 - display_pos) + i < 3)
//					displayWriteDig((3 - display_pos) + i,ID[i]);
//				else
//					displayClean((3 - display_pos) + i);
//			}
//			displayWriteDig(3,actua_value);		// Si encontro la posicion que se estaba modificando muestra la modificacion
//		}
//		else //si se escribieron mas de 4 numeros...
//		{
//			if(display_pos - disp_modif_pos > 0) //entonces necesitamos la posicion del digito a modificar.
//			{
//				int i;
//				for(i = 0; i < 3; i++)
//				{
//					displayWriteDig(i,ID[disp_modif_pos - (3 - i)]);
//				}
//				displayWriteDig(3,actua_value);
//			}
//			else
//			{
//				int i;
//				for(i = 4; i < display_pos; i++)
//				{
//					if(i == disp_modif_pos)
//						displayWriteDig(i - 4,actua_value);		// Si encontro la posicion que se estaba modificando muestra la modificacion
//					else
//						displayWriteDig(i - 4,ID[i]);
//				}
//				for(i = display_pos; i < ID_TOTAL_NUMBERS; i++)
//					displayClean(i - 4);
//			}
//		}
	}
}



ID_state_t get_ID_state(void)
{
	return ID_state;
}


bool get_ID_value(uint8_t* ID_array)
{
	if(ID_state == ID_READY)
	{
		int i;
		for(i = 0; i < ID_TOTAL_NUMBERS; i++)
			ID_array[i] = ID[i];
		ID_state = ID_NOT_READY;
		return 0;												// Devuelve 0 si todo OK
	}
	else
		return 1;
}


void restart_ID(void)
{
	int i;
	for(i = 0; i < ID_TOTAL_NUMBERS; i++)
		ID[i] = 0;
}

void ID_cancel_request(void)
{
	ID_state = ID_CANCELLED;
	display_pos  = 0;
	actua_value = 0;
	displayCleanAll();
	switch_st = 1;
}

void restart_ID_state(void)
{
	ID_state = ID_NOT_READY;
}

/********************************************************************************************/



void update_PIN(void)
{
	static bool cancel_req_on = false;

	if(check_new_twist())							// Si hubo un nuevo giro
	{
		if(cancel_req_on)
		{
			//TIMERstop(2);
			cancel_req_on = false;
		}
		if(get_last_twist() == CLOCKWISE && actua_value < 9)		// Incremento o decremento segun corresponda
			actua_value++;
		else if((get_last_twist() == ANTCLOCKWISE) && actua_value > 0)		// Si es menor a cero no lo vas a queres decrementar
			actua_value--;
	}

	if(check_new_switch()) {					// Si hubo una pulsacion
		if(get_last_switch() == DOWN)
		{
			PIN[display_pos] = actua_value;
			actua_value = 0;
			display_pos++;

			cancel_req_on = true;
			//TIMERloadISR(2, PIN_cancel_request, 3000, TIMER_ISRONCE);		// Se inicia el timer para la cancelacion
//			timersEnable();
			//TIMERstart(2);
		}
		else
		{
			//TIMERstop(2);
			cancel_req_on = false;
		}
	}

	if(display_pos == PIN_TOTAL_NUMBERS)			// Si ya se ingresaron todos los digitos se devuelve TRUE
	{
		actua_value = 0;
		display_pos = 0;
		PIN_state = PIN_READY;
		//TIMERstop(2);
		cancel_req_on = false;
	}
	else
	{
		switch(display_pos)
		{
		case 0:
			displayCleanDig(0);
			displayCleanDig(1);
			displayCleanDig(2);
			displayWriteChar(3, '-');
			break;
		case 1:
			displayCleanDig(0);
			displayCleanDig(1);
			displayWriteChar(2, '-');
			displayWriteChar(3, '-');
			break;
		case 2:
			displayCleanDig(0);
			displayWriteChar(1, '-');
			displayWriteChar(2, '-');
			displayWriteChar(3, '-');
			break;
		default:
			displayWriteChar(0, '-');
			displayWriteChar(1, '-');
			displayWriteChar(2, '-');
			displayWriteChar(3, '-');
			break;
		}
	}
}



PIN_state_t get_PIN_state(void)
{
	return PIN_state;
}


bool get_PIN_value(uint8_t* PIN_array)
{
	if(PIN_state == PIN_READY)
	{
		int i;
		for(i = 0; i < PIN_TOTAL_NUMBERS; i++)
			PIN_array[i] = PIN[i];
		PIN_state = PIN_NOT_READY;
		return 0;												// Devuelve 0 si todo OK
	}
	else
		return 1;
}


void restart_PIN(void)
{
	int i;
	for(i = 0; i < PIN_TOTAL_NUMBERS; i++)
		PIN[i] = 0;
	display_pos  = 0;
}

void PIN_cancel_request(void)
{
	PIN_state = PIN_CANCELLED;
	display_pos  = 0;
	actua_value = 0;
	displayCleanAll();
}

void restart_PIN_state(void)
{
	PIN_state = PIN_NOT_READY;
}
