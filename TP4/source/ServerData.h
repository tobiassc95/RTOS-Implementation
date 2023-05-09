/*
 * ServerData.h
 *
 *  Created on: 11 sept. 2019
 *      Author: root
 */

#ifndef SERVERDATA_H_
#define SERVERDATA_H_
#include "stdint.h"

typedef enum {CLIENT, ADMIN} user_status_t;				// el usuario puede ser cliente o administrador, empieza de 2 para que no se pise con el control

uint8_t Data_VerifyID(uint8_t * dataInput);
uint8_t Data_VerifyPIN(uint8_t * dataInput);
uint8_t Data_AddUser(uint8_t * idData, uint8_t * pinData, user_status_t user_status);
uint8_t Data_RemoveUser(uint8_t * idData);
uint8_t Data_Reset();
user_status_t Get_User_Status();


#endif /* SERVERDATA_H_ */
