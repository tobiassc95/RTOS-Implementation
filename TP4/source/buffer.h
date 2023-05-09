/*
 * bufferMacro.h
 *
 *  Created on: 30 ago. 2018
 *      Author: Ramiro Chiocci
 */


#include <stdint.h>

#ifndef name_BUFFERMACRO_C_
#define name_BUFFERMACRO_C_
#endif
#ifndef TRUE
#define TRUE (uint8_t) 1
#endif
#ifndef FALSE
#define FALSE (uint8_t) 0
#endif
#ifndef NULL
#define NULL (void*)0
#endif

typedef struct {
	void * data;
	void * bufferStartPtr;
	void * bufferEndPtr;
	uint8_t size;
	uint16_t lenght;
	uint8_t bufferQty;
	uint8_t fullBuffFg;
	uint8_t emptyBuffFg;
} bufferStruct;

#ifndef _BUFFER_C
	#define BUFFERMACRO(name,dataType,lenght_number)														\
		static dataType name##_data[lenght_number];															\
		static bufferStruct name = {	.data = (void *)name##_data, .bufferStartPtr = (void *)name##_data,	\
										.bufferEndPtr = (void *)name##_data, .size = sizeof(dataType), 		\
										.lenght = lenght_number, .bufferQty = 0,							\
										.fullBuffFg = FALSE, .emptyBuffFg = TRUE};							\
		static bufferStruct * name##_BUFF_PTR = &name;
#endif
uint8_t buffWrite (bufferStruct * buffer, void * data);
uint8_t buffRead (bufferStruct * buffer, void * data);
uint8_t isFullBuff (bufferStruct * buffer);
uint8_t isEmptyBuff (bufferStruct * buffer);
void flushBuff (bufferStruct * buffer);
uint16_t buffLenght(bufferStruct * buffer);

