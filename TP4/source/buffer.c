/*
 * bufferMacro.h
 *
 *  Created on: 30 ago. 2018
 *      Author: Ramiro Chiocci
 */
#define _BUFFER_C
#include <stdint.h>
#include <string.h>
#include "buffer.h"


uint8_t buffWrite (bufferStruct * buffer, void * data)
{
	void * endElementPtr = (buffer->data)+((buffer->size)*((buffer->lenght)-1));
	if (data == NULL)
		return (FALSE);
	if ((buffer->bufferEndPtr) == endElementPtr)
		buffer->bufferEndPtr = buffer->data;
	else
	{
		if (buffer->bufferQty != 0)
			buffer->bufferEndPtr += buffer->size;
	}

	if (buffer->bufferQty == buffer->lenght)
	{
		buffer->bufferStartPtr = buffer->bufferEndPtr;
		buffer->fullBuffFg = TRUE;
	}
	else
	{
		(buffer->bufferQty)++;
		buffer->fullBuffFg = FALSE;
	}

	memcpy(buffer->bufferEndPtr, data, buffer->size);
	buffer->emptyBuffFg = FALSE;
	return (TRUE);
}

uint8_t buffRead (bufferStruct * buffer, void * data)
{
	if (data == NULL)
		return (FALSE);
	else
	{
		void * endElementPtr = (buffer->data)+((buffer->size)*((buffer->lenght)-1));
		if (buffer->emptyBuffFg == TRUE)
			return (FALSE);
		memcpy(data,buffer->bufferStartPtr,buffer->size);
		(buffer->bufferQty)--;
		if (buffer->bufferStartPtr == endElementPtr)
			buffer->bufferStartPtr = buffer->data;
		else
		{
			if (buffer->bufferQty == 0)
				buffer->emptyBuffFg = TRUE;
			else
			{
				(buffer->bufferStartPtr) += (buffer->size);
				buffer->emptyBuffFg = FALSE;
			}
		}
		(buffer->fullBuffFg) = FALSE;
		return (TRUE);
	}
}

uint8_t isFullBuff (bufferStruct * buffer)
{
	if (buffer->fullBuffFg == TRUE)
		return (TRUE);
	else
		return (FALSE);
}

uint8_t isEmptyBuff (bufferStruct * buffer)
{
	if (buffer->emptyBuffFg == TRUE)
		return (TRUE);
	else
		return (FALSE);
}

void flushBuff (bufferStruct * buffer)
{
	if (buffer != NULL)
	{
		buffer->bufferStartPtr = buffer->data;
		buffer->bufferEndPtr = buffer->bufferStartPtr;
		buffer->bufferQty = 0;
		buffer->fullBuffFg = FALSE;
		buffer->emptyBuffFg = TRUE;
	}
	return;
}

uint16_t buffLenght(bufferStruct * buffer)
{
	return (buffer->bufferQty);
}
