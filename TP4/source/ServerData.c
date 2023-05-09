/*
 * ServerData.c
 *
 *  Created on: 11 sept. 2019
 *      Author: root
 */
#include "ServerData.h"
#include "buffer.h"
#include "string.h"
#define USER_CANT 10
#define ID_SIZE 8
#define PIN_SIZE 4

typedef struct {
	uint8_t ID_NUMBER[8];
	uint8_t PIN_NUMBER[4];
	user_status_t user_status;
}Data_t;

BUFFERMACRO(SERVER,Data_t,USER_CANT)
static Data_t data;
static uint8_t idInput[ID_SIZE];
static uint8_t pinInput[PIN_SIZE];
static uint8_t flagUserActive;

uint8_t Data_Reset()
{
	uint8_t k;
	for (k = 0; k < ID_SIZE; k++)
		data.ID_NUMBER[k] = 0;
	for (k = 0; k < PIN_SIZE; k++)
		data.PIN_NUMBER[k] = 0;
	flagUserActive = FALSE;
	return (0);
}
uint8_t Data_VerifyID(uint8_t * dataInput)
{
	uint8_t k;
	for (k = 0; k < ID_SIZE; k++)
		idInput[k] = dataInput[k];
	for (k = 0; k < buffLenght(SERVER_BUFF_PTR); k++)
	{
		buffRead(SERVER_BUFF_PTR, (void *) (&data));
		if (memcmp((void *)idInput,(void *)(data.ID_NUMBER),ID_SIZE) == 0)
		{
			flagUserActive = TRUE;
			buffWrite(SERVER_BUFF_PTR, (void *) (&data));
			return (0);
		}
		else
			buffWrite(SERVER_BUFF_PTR, (void *) (&data));
	}
	return (1);
}

uint8_t Data_VerifyPIN(uint8_t * dataInput)
{
	uint8_t k;
	for (k = 0; k < PIN_SIZE; k++)
		pinInput[k] = dataInput[k];
	if ((memcmp((void *)pinInput,(void *)(data.PIN_NUMBER),PIN_SIZE) == 0) && (flagUserActive == TRUE))
		return (0);
	else {
//		Data_Reset();
		return (1);
	}
}

uint8_t Data_AddUser(uint8_t * idData, uint8_t * pinData, user_status_t user_status)
{
	Data_t new;
	uint8_t k;
	for (k = 0; k < ID_SIZE; k++)
		new.ID_NUMBER[k] = idData[k];
	for (k = 0; k < PIN_SIZE; k++)
		new.PIN_NUMBER[k] = pinData[k];
	new.user_status = user_status;
	buffWrite(SERVER_BUFF_PTR, (void *)(&new));
	return (0);
}

uint8_t Data_RemoveUser(uint8_t * idData)
{
	uint8_t k;
	for (k = 0; k < ID_SIZE; k++)
		idInput[k] = idData[k];
	for (k = 0; k < buffLenght(SERVER_BUFF_PTR); k++) {
		buffRead(SERVER_BUFF_PTR, (void *) (&data));
		if (memcmp((void *)idInput,(void *)(data.ID_NUMBER),ID_SIZE) == 0)
			return (0);
		else {
			buffWrite(SERVER_BUFF_PTR, (void *) (&data));
		}
	}
	return (1);
}

user_status_t Get_User_Status()
{
	if(flagUserActive == TRUE)
	{
		Data_Reset();
		return data.user_status;
	}
	else
		return 1;
}
