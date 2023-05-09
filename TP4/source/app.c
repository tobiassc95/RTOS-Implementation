#include "os.h"
#include "portpin.h"
#include "display.h"
#include "encoder.h"
#include "EncProcess.h"
#include "ledControl.h"
#include "ServerData.h"

//led task
#define LEDTASK_STKSIZ	256u //stack size.
#define LEDTASK_STKSIZ_LIMIT (APPTASK_STKSIZ/10) //watermark limit.
#define LEDTASK_PRIOR 3u //priority.

enum { //STATES
	RESETALL,
	IDLE, 			//estado inicial. queda en espera.
	WRITEID, 		//escribimos ID.
	VALIDATEID, 	//validamos ID.
	WRITEPIN, 		//lee el pin una vez digitado.
	VALIDATEPIN, 	//validamos pin.
	GET_USER_STAT,	//obtenemos el estatus del usuario (ADMIN o CLIENTE)
	ADMIN_OP,		//el admin decide que operacion hacer
	NEW_USER_ID,	//el admin agrega un usuario
	NEW_USER_PIN,
	ADD_USER,
	REMOVE_USER,	//el admin borra un usuario
	ERROR, 			//ID o PIN erroneos.
	OPENDOOR, 		//abre la puerta y el display dice "bye ".
	CANCEL,
};

//para el timer.
void resetAll();
void cancelAll (void);
//para la fsm.
void waitEvent (void);
void waitID (void);
void validateID (void);
void waitPIN (void);
void validatePIN (void);
void waitError (void);
void waitDoor (void);
void waitAdmin (void);
void get_user_stat(void);
void waitAddUser_ID (void);
void waitAddUser_PIN (void);
void waitAddUser (void);
void waitRemoveUser (void);

//secuencia de luces de leds
void ledSeq(void *p_arg);
//void delayLoop(uint32_t mili_Seg);

//led task
static OS_TCB ledtask_tcb; //task information handled by the OS.
static CPU_STK ledtask_stk[LEDTASK_STKSIZ];
//semasphore
static OS_SEM fsmsem;
static OS_SEM ledsem;

//variable y arreglo globales para la fsm.
static uint8_t state;
void (*FSMfunc[]) (void) = {
		resetAll,
		waitEvent,								//0
		waitID,									//1
		validateID,								//2
		waitPIN,								//3
		validatePIN,							//4
		get_user_stat,							//5
		waitAdmin,								//6
		waitAddUser_ID,							//7
		waitAddUser_PIN,						//8
		waitAddUser,							//9
		waitRemoveUser,							//10
		waitError,								//11
		waitDoor,								//12
		cancelAll								//13
};

//arreglos globales para la guardar los datos ingresados.
static uint8_t userIDdata[19];
static uint8_t userPINdata[4];

//variables para realizar pruebas
static uint8_t user_ID_1[8];
static uint8_t user_PIN_1[4];

//para saber si se accedio
static bool access;

/* Función que se llama 1 vez, al comienzo del programa */
void appinit (void) {
	OS_ERR oserr;
	OSTaskCreate(&ledtask_tcb,"leds task",ledSeq,0,LEDTASK_PRIOR,&ledtask_stk[0],(LEDTASK_STKSIZ/10),LEDTASK_STKSIZ,0,0,0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),&oserr);
	OSSemCreate(&fsmsem, "ftm sem", 0, &oserr);
	OSSemCreate(&ledsem, "leds sem", 0, &oserr);

	state = RESETALL; //estado inicial.
	displayConfiguration();
//	decodeCard_Init();
	init_encoder();
	enc_set_sem(&fsmsem);
	led_init();

	int i;
	for(i = 0; i < 8; i++)
		user_ID_1[i] = i;					// ID de usuario de prueba: ID = [0,1,2,3,4,5,6,7]
	for(i = 0; i < 4; i++)
			user_PIN_1[i] = i;				// PIN de usuario de prueba: ID = [0,1,2,3]
	Data_AddUser(user_ID_1, user_PIN_1, ADMIN);	// Agrego el usuario de prueba
}

/* Función que se llama constantemente en un ciclo infinito */
void apptask (void *p_arg) {
	OS_ERR oserr;
	(void)p_arg;

	while (1) {
		OSSemPend(&fsmsem, 0, OS_OPT_PEND_BLOCKING, 0, &oserr); //if semasphore is set, clear.
		FSMfunc[state]();
	}
}

void resetAll(void) {
	displayCleanAll();
//	decodeCard_enable();
//	encoder_enable();
	state = IDLE;
}

//funciones de la fsm.
void waitEvent(void) {
	if (check_new_switch())
	{
		//loadISRtimer(0, cancelAll, 60000, ISR_); //se llama la funcion cuando paso 1 min de inactividad.
//			decodeCard_disable();
//			timersEnable();
//			displayEnable();
			//habilitamos timers para operar sobre el PIN.??
			//Deshabilitamos el CARDREADER.
			//timerStart(0);
			state = WRITEID;
	}
//	else if (decodeCard_IsDataReady()) {
//		if(~decodeCard_Error()) {
			//loadISRtimer(0, cancelAll, 60000, ISR_); //se llama la funcion cuando paso 1 min de inactividad.
//			timersEnable();
//			displayEnable();
			//habilitamos timers para operar sobre el PIN.??
			//deshabilitamos ID.
//			decodeCard_GetPan(userIDdata);
//			decodeCard_Reset();
			//timerStart(0);
//			state = VALIDATEID;
//		}
//		else
//		{
//			state = RESETALL;
//		}
//	}
}

void waitID (void) { //es un timer periodico.
	//habilitamos timers para operar sobre el PIN.??
	//extraemos y guardamos datos de la tarjeta.
	switch(get_ID_state()) {
	case ID_NOT_READY:
		update_ID();
		break;
	case ID_READY:
		get_ID_value(userIDdata);
		state = VALIDATEID;
		break;
	case ID_CANCELLED:
		state = RESETALL;
		break;
	}
}

void validateID (void) {
	if(Data_VerifyID(userIDdata) == 0)
		state = WRITEPIN;
	else
		state = ERROR;
}

void waitPIN (void) {
	switch(get_PIN_state())
	{
	case PIN_NOT_READY:
		update_PIN();
		break;
	case PIN_READY:
		get_PIN_value(userPINdata);
		state = VALIDATEPIN;
		break;
	case PIN_CANCELLED: //el usuario decide cancelar la operacion.
		state = RESETALL;
		break;
	}
}

void validatePIN (void) {
	if(Data_VerifyPIN(&userPINdata[0]) == 0)
		state = OPENDOOR;
	else
		state = ERROR;
}

void waitError() {
	OS_ERR oserr;
	access = false;
	OSSemPost(&ledsem, OS_OPT_POST_1, &oserr); //set semasphore.
	//reseteamos TODO.
	state = RESETALL;
}

void waitDoor (void) {
	OS_ERR oserr;
	access = true;
	OSSemPost(&ledsem, OS_OPT_POST_1, &oserr); //set semasphore.
	//reseteamos TODO.
	state = RESETALL;
}

void cancelAll (void) {
	//reseteamos TODO.
	restart_ID();
	restart_PIN();
	if(check_new_switch())
		if(get_last_switch() == HIGH)
			state = RESETALL;
}

void get_user_stat(void)
{
	user_status_t user_stat;
	user_stat = Get_User_Status();

	switch(user_stat)
	{
		case ADMIN:
			state = ADMIN_OP;
			break;
		case CLIENT:
			state = OPENDOOR;
	}
}

void waitAdmin (void)
{
	static uint8_t temp = NEW_USER_ID;
	static bool entered = false;
	static bool debug = false;

	if(!entered)
	{
		displayWriteStr("Add");
		entered = true;
		if(get_last_switch() == LOW)				// Puede pasar que cuando se llego a este estado el switch todavia esta presionado
			debug = true;							// entonces cuando el usuario lo suelte inmediatamente va a salir d esta funcion
	}												// hacendo esto prevengo eso

	if(check_new_twist())
	{
		if(temp == NEW_USER_ID)
		{
			temp = REMOVE_USER;
			displayWriteStr("dele");
		}
		else if(temp == REMOVE_USER)
		{
			temp = OPENDOOR;
			displayWriteStr("Open");
		}
		else if(temp == OPENDOOR)
		{
			temp = NEW_USER_ID;
			displayWriteStr("Add");
		}
	}
	if (check_new_switch())
	{
		if(get_last_switch() == HIGH)
		{
			if(debug)
				debug = false;
			else
			{
				entered = false;
				state = temp;
			}
		}
	}
}

void waitAddUser_ID (void)
{
	switch(get_ID_state())
		{
		case ID_NOT_READY:
			update_ID();
			break;
		case ID_READY:
			get_ID_value(userIDdata);
			restart_ID();
			state = NEW_USER_PIN;
			break;
		case ID_CANCELLED:
			state = CANCEL;
			break;
		}
}

void waitAddUser_PIN (void)
{
	switch(get_PIN_state())
		{
		case PIN_NOT_READY:
			update_PIN();
			break;
		case PIN_READY:
			get_PIN_value(userPINdata);
			restart_PIN();
			state = ADD_USER;
			break;
		case PIN_CANCELLED:
			state = CANCEL;
			break;
		}
}


void waitAddUser (void)
{
	Data_AddUser(userIDdata, userPINdata, CLIENT);
	restart_ID();
	restart_PIN();
	if(check_new_switch())
		if(get_last_switch() == HIGH)
			state = RESETALL;
}

void waitRemoveUser (void)
{

	switch(get_ID_state())
		{
		case ID_NOT_READY:
			update_ID();
			break;
		case ID_READY:
			get_ID_value(userIDdata);
			Data_RemoveUser(userIDdata);
			state = RESETALL;
			break;
		case ID_CANCELLED:
			state = CANCEL;
			break;
		}
}




/****************************** FUNCIONES PARA LAS SECUENCIAS DE LUCES DE LEDS *******************************/

void ledSeq(void *p_arg) {
	OS_ERR oserr;
	int i;
	(void)p_arg;

	while (1) {
		OSSemPend(&ledsem, 0, OS_OPT_PEND_BLOCKING, 0, &oserr); //if semasphore is set, clear.
		if(access) {
			for(i = 0; i < 5; i++) {
				D1_ON();
				OSTimeDly(300, OS_OPT_TIME_DLY, &oserr); //300 ticks = 300msec
				D2_ON();
				OSTimeDly(300, OS_OPT_TIME_DLY, &oserr); //300 ticks = 300msec
				D3_ON();
				OSTimeDly(300, OS_OPT_TIME_DLY, &oserr); //300 ticks = 300msec
			}
		}
		else {
			for(i = 0; i < 4; i++) {
				D1_ON();
				D2_ON();
				D3_ON();
				OSTimeDly(500, OS_OPT_TIME_DLY, &oserr); //500 ticks = 500msec
				leds_OFF();
				OSTimeDly(500, OS_OPT_TIME_DLY, &oserr); //500 ticks = 500msec
			}
		}
	}
}

//void delayLoop(uint32_t mili_Seg)
//{
//	mili_Seg = 14825*mili_Seg;
//	while(mili_Seg)
//		mili_Seg--;
//}
