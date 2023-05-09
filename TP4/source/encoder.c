/*
 * encoder.c
 *
 *  Created on: 2 sep. 2019
 *      Author: guido
 */

#include "encoder.h"
#include "portpin.h"
//#include "timer.h"


#define PIN_CHA			PORTNUM2PIN(PORT_B,2)			// Pines gpio del puerto B
#define PIN_CHB			PORTNUM2PIN(PORT_B,3)
#define PIN_SWITCH		PORTNUM2PIN(PORT_B,10)

#define MILISECOND		14825

typedef enum {CHA, CHB, SWITCH} pin_t;						// Señales del encoder
typedef enum {FALLING_, LOW_, RISING_, HIGH_} state_t;		// Estados posibles de las señales

typedef struct
{
	state_t state;
	pin_t pin;
} pin_data_t;

typedef struct
{
	pin_data_t channel_a;
	pin_data_t channel_b;
	pin_data_t switch_;
} encoder_data_t;

static encoder_data_t encoder_data;			// Estructura global de los datos del encoder
static bool new_twist;						// Booleanos global para avisar cuando hubo cambios
static bool new_switch;
//static twist_dir_t event;			// Sentido del utimo giro
static bool cha_ready;						// flags para saber cuando subieron los canales
static bool chb_ready;
static twist_dir_t twist_direction;			// Sentido del utimo giro
static enc_event_t last_event;
static bool queue_setted;
static bool sem_setted;

/************************* FUNCIONES GLOBALES QUE VA A SER PARA CALLBACKS ***********************/

void set_cha_falling(void);
void set_chb_falling(void);

void set_cha_low(void);
void set_chb_low(void);

void set_cha_rising(void);
void set_chb_rising(void);

void set_cha_high(void);
void set_chb_high(void);

void set_switch_high(void);
void set_switch_low(void);

void encoder_callback1(void);					// callbacks respectivos para cada una de las señales
void encoder_callback2(void);
void encoder_callback3(void);

static void wait_and_handle_chA_fall(void *p_arg);			// Para realizar el debouncing de las señales
static void wait_and_handle_chA_rise(void *p_arg);
static void wait_and_handle_chB_fall(void *p_arg);			// Se esperara un determinado tiempo y se obserbara el valor de la señal
static void wait_and_handle_chB_rise(void *p_arg);			// para, de ese modo, evitar el rebote
static void wait_and_handle_swt_fall(void *p_arg);
static void wait_and_handle_swt_rise(void *p_arg);

/*************************************************************************************************/

/********************************* FUNCIONES, MACROS Y VARIABLES DE RTOS ************************************/

void init_RTOS_tasks(void);

#define WAIT_AND_HANDLE_STK_SIZE 		512u
#define WAIT_AND_HANDLE_PRIO 			2u

OS_Q* osQueue;
OS_SEM* osSem;

static OS_TCB chA_fallTCB;
static CPU_STK chA_fall_Stk[WAIT_AND_HANDLE_STK_SIZE];
static OS_SEM sem_chA_fall;
OS_ERR os_err_chA_fall;

static OS_TCB chA_riseTCB;
static CPU_STK chA_rise_Stk[WAIT_AND_HANDLE_STK_SIZE];
static OS_SEM sem_chA_rise;
OS_ERR os_err_chA_rise;

static OS_TCB chB_fallTCB;
static CPU_STK chB_fall_Stk[WAIT_AND_HANDLE_STK_SIZE];
static OS_SEM sem_chB_fall;
OS_ERR os_err_chB_fall;

static OS_TCB chB_riseTCB;
static CPU_STK chB_rise_Stk[WAIT_AND_HANDLE_STK_SIZE];
static OS_SEM sem_chB_rise;
OS_ERR os_err_chB_rise;

static OS_TCB swt_fallTCB;
static CPU_STK swt_fall_Stk[WAIT_AND_HANDLE_STK_SIZE];
static OS_SEM sem_swt_fall;
OS_ERR os_err_swt_fall;

static OS_TCB swt_riseTCB;
static CPU_STK swt_rise_Stk[WAIT_AND_HANDLE_STK_SIZE];
static OS_SEM sem_swt_rise;
OS_ERR os_err_swt_rise;

/*************************************************************************************************/















/*************************************************************************************************/

void init_encoder(void)							// Se inicializa el encoder
{
	PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_FALLING);
	PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_FALLING);
	PINconfigure(PIN_SWITCH, PIN_MUX1, PIN_IRQ_FALLING);

	PINmode(PIN_CHA, PIN_INPUT);
	PINmode(PIN_CHB, PIN_INPUT);
	PINmode(PIN_SWITCH, PIN_INPUT);

	PINpull(PIN_CHA, PIN_PULLDOWN);
	PINpull(PIN_CHB, PIN_PULLDOWN);
	PINpull(PIN_SWITCH, PIN_PULLDOWN);

	PINloadISR(2, encoder_callback1, PIN_CHA); // Se asume que cuando se inicia el dispositivo los valoes
	PINloadISR(4, encoder_callback2, PIN_CHB); // de todos los pines va a ser HIGH
	PINloadISR(3, encoder_callback3, PIN_SWITCH); // con lo cual debe estar atento a las caidas

	encoder_data.channel_a.pin = CHA;
	encoder_data.channel_b.pin = CHB;
	encoder_data.switch_.pin = SWITCH;

	encoder_data.channel_a.state = HIGH_;
	encoder_data.channel_b.state = HIGH_;
	encoder_data.switch_.state = HIGH_;

	new_switch = false;
	new_twist = false;

	queue_setted = false;
	sem_setted = false;

	cha_ready = false;													// Los channels empiezan arriba pero se setean como
	cha_ready = false;													// false para que no hallan conflictos

	encoder_enable();

	init_RTOS_tasks();
}

void encoder_enable(void)						// Se habilita el encoder
{
	enablePINinterrupt(PIN_CHA);
	enablePINinterrupt(PIN_CHB);
	enablePINinterrupt(PIN_SWITCH);
}

void encoder_disable(void)
{
	disablePINinterrupt(PIN_CHA);
	disablePINinterrupt(PIN_CHB);
	disablePINinterrupt(PIN_SWITCH);
}

/****************************************************************************************************************
 *****************************************************************************************************************
 *									DEFINICION DE FUNCIONES LOCALES												*
 ****************************************************************************************************************
 ****************************************************************************************************************/

void set_channels_high(void)
{
	encoder_data.channel_a.state = HIGH_;
	encoder_data.channel_b.state = HIGH_;
	PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_LOGIC0);
	PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_LOGIC0);
//	pinISFclean(PIN_CHA);
//	pinISFclean(PIN_CHB);
	new_twist = true;
}

void set_cha_falling(void)
{
//	pinISFclean(PIN_CHB);
	encoder_data.channel_a.state = FALLING_;
	PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_LOGIC0);
}

void set_chb_falling(void)
{
//	pinISFclean(PIN_CHA);
	encoder_data.channel_b.state = FALLING_;
	PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_LOGIC0);
}

void set_cha_low(void)
{
//	pinISFclean(PIN_CHB);
	PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_DISABLE);
	PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_DISABLE);
	OSSemPost(&sem_chA_fall, OS_OPT_POST_1, &os_err_chA_fall);		// Se pone en post el semaforo para que se inicie el taskHandler de chA_fall
//	TIMERloadISR(1, wait_and_handle_chA_fall, 7, TIMER_ISRONCE);
//	timersEnable();
//	TIMERstart(1);
}

void set_chb_low(void)
{
//	pinISFclean(PIN_CHA);
	PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_DISABLE);
	PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_DISABLE);
	OSSemPost(&sem_chB_fall, OS_OPT_POST_1, &os_err_chB_fall);
//	TIMERloadISR(1, wait_and_handle_chB_fall, 7, TIMER_ISRONCE);
//	timersEnable();
//	TIMERstart(1);
}

void set_cha_rising(void)
{
	encoder_data.channel_a.state = RISING_;
	PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_LOGIC1);
}

void set_chb_rising(void)
{
	encoder_data.channel_b.state = RISING_;
	PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_LOGIC1);
}

void set_cha_high(void)
{
//	pinISFclean(PIN_CHB);
	PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_DISABLE);
	OSSemPost(&sem_chA_rise, OS_OPT_POST_1, &os_err_chA_rise);
//	TIMERloadISR(1, wait_and_handle_chA_rise, 7, TIMER_ISRONCE);
//	timersEnable();
//	TIMERstart(1);
}

void set_chb_high(void)
{
//	pinISFclean(PIN_CHA);
	PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_DISABLE);
	OSSemPost(&sem_chB_rise, OS_OPT_POST_1, &os_err_chB_rise);
//	TIMERloadISR(1, wait_and_handle_chB_rise, 7, TIMER_ISRONCE);
//	timersEnable();
//	TIMERstart(1);
}

void set_switch_high(void)
{
	PINconfigure(PIN_SWITCH, PIN_MUX1, PIN_IRQ_DISABLE);
	OSSemPost(&sem_swt_rise, OS_OPT_POST_1, &os_err_swt_rise);
//	TIMERloadISR(1, wait_and_handle_swt_rise, 7, TIMER_ISRONCE);
//	timersEnable();
//	TIMERstart(1);
}

void set_switch_low(void)
{
	PINconfigure(PIN_SWITCH, PIN_MUX1, PIN_IRQ_DISABLE);
	OSSemPost(&sem_swt_fall, OS_OPT_POST_1, &os_err_swt_fall);
//	TIMERloadISR(1, wait_and_handle_swt_fall, 7, TIMER_ISRONCE);
//	timersEnable();
//	TIMERstart(1);
}

/****************************************************************************************************************
 *****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************/

void encoder_callback1(void)
{
//	bool flag_status_cha = PORT_StatusInterruptFlag(PIN_CHA);
//	bool flag_status_chb = PORT_StatusInterruptFlag(PIN_CHB);
//	bool flag_status_switch = PORT_StatusInterruptFlag(PIN_SWITCH);

//	pinISFclean(PIN_CHA);

	switch(encoder_data.channel_a.state)
	{
	case FALLING_:												// Si previamente estaba cayendo ahora esta abajo
		set_cha_low();
		break;
	case LOW_:													// Si estaba abajo ahora esta subiendo
		set_cha_rising();
		break;
	case RISING_:												// Si estaba subiendo ahora esta arriba
		set_cha_high();
		break;
	case HIGH_:													// Si estaba arriba ahora esta cayendo
		set_cha_falling();
		break;
	}
}

void encoder_callback2(void)
{
//	pinISFclean(PIN_CHB);

	switch(encoder_data.channel_b.state)
	{
	case FALLING_:												// Si previamente estaba cayendo ahora esta abajo
		set_chb_low();
		break;
	case LOW_:													// Si estaba abajo ahora esta subiendo
		set_chb_rising();
		break;
	case RISING_:												// Si estaba subiendo ahora esta arriba
		set_chb_high();
		break;
	case HIGH_:													// Si estaba arriba ahora esta cayendo
		set_chb_falling();
		break;
	}
}

void encoder_callback3(void)
{
//	pinISFclean(PIN_SWITCH);

	switch(encoder_data.switch_.state)
	{
	case LOW_:
		set_switch_high();
		break;
	case HIGH_:
		set_switch_low();
		break;
	default:
		break;
	}
}

bool check_new_twist(void)
{
	if(cha_ready || chb_ready)
	{
		cha_ready = false;
		chb_ready = false;
		return 1;
	}
	else
		return 0;
}

bool check_new_switch(void)
{
	if(new_switch)
	{
		new_switch = false;
		return 1;
	}
	else
		return 0;
}

twist_dir_t get_last_twist(void)
{
	return twist_direction;
}

switch_state_t get_last_switch(void)
{
	uint8_t ret;

	if(encoder_data.switch_.state == LOW_)
		ret = 0;
	else if(encoder_data.switch_.state == HIGH_)
		ret = 1;

	return ret;
}

static void wait_and_handle_chA_fall(void *p_arg)
{
	(void)p_arg; //arg pointer.
	OS_ERR os_err; //in cause there are errors in the CPU.

	while (1)
	{
		OSSemPend(&sem_chA_fall, 0, OS_OPT_PEND_BLOCKING, 0, &os_err);	// Queda pendiente del semaforo para iniciar la rutina
	    OSTimeDlyHMSM(0u, 0u, 0u, 7u, OS_OPT_TIME_HMSM_STRICT, &os_err);// Se espera 7 miliseg para iniciar la rutina (debouncing)

	    if(PINread(PIN_CHA) == LOW)													// Si efectivamente la señal bajo prosigo
		{
			PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_DISABLE);								// Cuando ve que cayo el A ya no da bola al B
			encoder_data.channel_a.state = LOW_;
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_RISING);
			twist_direction = ANTCLOCKWISE;
			last_event = ANTCLOCKWISE_EV;
		}
		else																			// Si la señal en realidad no bajo reseteo
		{
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_FALLING);
			PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_FALLING);
			encoder_data.channel_a.state = HIGH_;
		}
	}
}

static void wait_and_handle_chA_rise(void *p_arg)
{
	(void)p_arg; //arg pointer.
	OS_ERR os_err; //in cause there are errors in the CPU.

	while (1)
	{
		OSSemPend(&sem_chA_rise, 0, OS_OPT_PEND_BLOCKING, 0, &os_err);	// Queda pendiente del semaforo para iniciar la rutina
	    OSTimeDlyHMSM(0u, 0u, 0u, 7u, OS_OPT_TIME_HMSM_STRICT, &os_err);// Se espera 7 miliseg para iniciar la rutina (debouncing)

		if(PINread(PIN_CHA) == HIGH)												// Si efectivamente la señal subio reinicio el siclo
		{
			encoder_data.channel_b.state = HIGH_;
			encoder_data.channel_a.state = HIGH_;
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_FALLING);
			PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_FALLING);
			cha_ready = true;
			if(queue_setted)
				OSQPost(osQueue, &last_event, sizeof(last_event), OS_OPT_POST_ALL, &os_err);
			if(sem_setted)
				OSSemPost(osSem, OS_OPT_POST_1, &os_err);
		}
		else																			// Si la señal en realidad no subio vuelvo
		{
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_RISING);
			encoder_data.channel_a.state = LOW_;
		}
	}
}

static void wait_and_handle_chB_fall(void *p_arg)
{
	(void)p_arg; //arg pointer.
	OS_ERR os_err; //in cause there are errors in the CPU.

	while (1)
	{
		OSSemPend(&sem_chB_fall, 0, OS_OPT_PEND_BLOCKING, 0, &os_err);	// Queda pendiente del semaforo para iniciar la rutina
	    OSTimeDlyHMSM(0u, 0u, 0u, 7u, OS_OPT_TIME_HMSM_STRICT, &os_err);// Se espera 7 miliseg para iniciar la rutina (debouncing)

		if(PINread(PIN_CHB) == LOW)													// Si efectivamente la señal bajo prosigo
		{
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_DISABLE);								// Cuando ve que cayo el A ya no da bola al B
			encoder_data.channel_b.state = LOW_;
			PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_RISING);
			twist_direction = CLOCKWISE;
			last_event = CLOCKWISE_EV;
		}
		else																			// Si la señal en realidad no bajo reseteo
		{
			PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_FALLING);
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_FALLING);
			encoder_data.channel_b.state = HIGH_;
		}
	}
}

static void wait_and_handle_chB_rise(void *p_arg)
{
	(void)p_arg; //arg pointer.
	OS_ERR os_err; //in cause there are errors in the CPU.

	while (1)
	{
		OSSemPend(&sem_chB_rise, 0, OS_OPT_PEND_BLOCKING, 0, &os_err);	// Queda pendiente del semaforo para iniciar la rutina
	    OSTimeDlyHMSM(0u, 0u, 0u, 7u, OS_OPT_TIME_HMSM_STRICT, &os_err);// Se espera 7 miliseg para iniciar la rutina (debouncing)

		if(PINread(PIN_CHB) == HIGH)												// Si efectivamente la señal subio reinicio el siclo
		{
			encoder_data.channel_a.state = HIGH_;
			encoder_data.channel_b.state = HIGH_;
			PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_FALLING);
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_FALLING);
			chb_ready = true;
			if(queue_setted)
				OSQPost(osQueue, &last_event, sizeof(last_event), OS_OPT_POST_ALL, &os_err);
			if(sem_setted)
				OSSemPost(osSem, OS_OPT_POST_1, &os_err);
		}
		else																			// Si la señal en realidad no subio vuelvo
		{
			PINconfigure(PIN_CHB, PIN_MUX1, PIN_IRQ_RISING);
			PINconfigure(PIN_CHA, PIN_MUX1, PIN_IRQ_FALLING);
			encoder_data.channel_b.state = LOW_;
		}
	}
}

static void wait_and_handle_swt_fall(void *p_arg)
{
	(void)p_arg; //arg pointer.
	OS_ERR os_err; //in cause there are errors in the CPU.

	while (1)
	{
		OSSemPend(&sem_swt_fall, 0, OS_OPT_PEND_BLOCKING, 0, &os_err);	// Queda pendiente del semaforo para iniciar la rutina
	    OSTimeDlyHMSM(0u, 0u, 0u, 7u, OS_OPT_TIME_HMSM_STRICT, &os_err);// Se espera 7 miliseg para iniciar la rutina (debouncing)

		if(PINread(PIN_SWITCH) == LOW)
		{
			encoder_data.switch_.state = LOW_;
			PINconfigure(PIN_SWITCH, PIN_MUX1, PIN_IRQ_RISING);
			new_switch = true;
			last_event = DOWN;
			if(queue_setted)
				OSQPost(osQueue, &last_event, sizeof(last_event), OS_OPT_POST_ALL, &os_err);
			if(sem_setted)
				OSSemPost(osSem, OS_OPT_POST_1, &os_err);
		}
		else

		{
			encoder_data.switch_.state = HIGH_;
			PINconfigure(PIN_SWITCH, PIN_MUX1, PIN_IRQ_FALLING);
		}
	}
}

static void wait_and_handle_swt_rise(void *p_arg)
{
	(void)p_arg; //arg pointer.
	OS_ERR os_err; //in cause there are errors in the CPU.

	while (1)
	{
		OSSemPend(&sem_swt_rise, 0, OS_OPT_PEND_BLOCKING, 0, &os_err);	// Queda pendiente del semaforo para iniciar la rutina
	    OSTimeDlyHMSM(0u, 0u, 0u, 7u, OS_OPT_TIME_HMSM_STRICT, &os_err);// Se espera 7 miliseg para iniciar la rutina (debouncing)

		if(PINread(PIN_SWITCH) == HIGH)
		{
			encoder_data.switch_.state = HIGH_;
			PINconfigure(PIN_SWITCH, PIN_MUX1, PIN_IRQ_FALLING);
			new_switch = true;
			last_event = UP;
			if(queue_setted)
				OSQPost(osQueue, &last_event, sizeof(last_event), OS_OPT_POST_ALL, &os_err);
			if(sem_setted)
				OSSemPost(osSem, OS_OPT_POST_1, &os_err);
		}
		else
		{
			encoder_data.switch_.state = LOW_;
			PINconfigure(PIN_SWITCH, PIN_MUX1, PIN_IRQ_RISING);
		}
	}
}


void init_RTOS_tasks(void)
{
	OS_ERR os_err;						// ESTARA BIEN QUE CREE ESTAS VARIABLES ACA ?

//	OSInit(&os_err);					// ESTARA BIEN QUE CREE ESTAS VARIABLES ACA ?

	//OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz);	// ASUMO QUE ESTO YA SE CREO

	OSSemCreate(&sem_chA_fall, "sem chA fall", 0u, &os_err_chA_fall);
	OSSemCreate(&sem_chA_rise, "sem chA rise", 0u, &os_err_chA_rise);
	OSSemCreate(&sem_chB_fall, "sem chB fall", 0u, &os_err_chB_fall);
	OSSemCreate(&sem_chB_rise, "sem chB rise", 0u, &os_err_chB_rise);
	OSSemCreate(&sem_swt_fall, "sem swt fall", 0u, &os_err_swt_fall);
	OSSemCreate(&sem_swt_rise, "sem swt rise", 0u, &os_err_swt_rise);


	OSTaskCreate(&chA_fallTCB,
	                 "chA fall",
	                  wait_and_handle_chA_fall,
	                  0u,
	                  WAIT_AND_HANDLE_PRIO,
	                 &chA_fall_Stk[0u],
	                 (WAIT_AND_HANDLE_STK_SIZE / 10u),
					 WAIT_AND_HANDLE_STK_SIZE,
	                  0u,
	                  0u,
	                  0u,
	                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
	                 &os_err);

	OSTaskCreate(&chA_riseTCB,
		                 "chA rise",
		                  wait_and_handle_chA_rise,
		                  0u,
		                  WAIT_AND_HANDLE_PRIO,
		                 &chA_rise_Stk[0u],
		                 (WAIT_AND_HANDLE_STK_SIZE / 10u),
						 WAIT_AND_HANDLE_STK_SIZE,
		                  0u,
		                  0u,
		                  0u,
		                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
		                 &os_err);

	OSTaskCreate(&chB_fallTCB,
		                 "chB fall",
		                  wait_and_handle_chB_fall,
		                  0u,
		                  WAIT_AND_HANDLE_PRIO,
		                 &chB_fall_Stk[0u],
		                 (WAIT_AND_HANDLE_STK_SIZE / 10u),
						 WAIT_AND_HANDLE_STK_SIZE,
		                  0u,
		                  0u,
		                  0u,
		                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
		                 &os_err);

	OSTaskCreate(&chB_riseTCB,
		                 "chB rise",
		                  wait_and_handle_chB_rise,
		                  0u,
		                  WAIT_AND_HANDLE_PRIO,
		                 &chB_rise_Stk[0u],
		                 (WAIT_AND_HANDLE_STK_SIZE / 10u),
						 WAIT_AND_HANDLE_STK_SIZE,
		                  0u,
		                  0u,
		                  0u,
		                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
		                 &os_err);

	OSTaskCreate(&swt_fallTCB,
		                 "swt fall",
		                  wait_and_handle_swt_fall,
		                  0u,
		                  WAIT_AND_HANDLE_PRIO,
		                 &swt_fall_Stk[0u],
		                 (WAIT_AND_HANDLE_STK_SIZE / 10u),
						 WAIT_AND_HANDLE_STK_SIZE,
		                  0u,
		                  0u,
		                  0u,
		                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
		                 &os_err);

	OSTaskCreate(&swt_riseTCB,
		                 "swt rise",
		                  wait_and_handle_swt_rise,
		                  0u,
		                  WAIT_AND_HANDLE_PRIO,
		                 &swt_rise_Stk[0u],
		                 (WAIT_AND_HANDLE_STK_SIZE / 10u),
						 WAIT_AND_HANDLE_STK_SIZE,
		                  0u,
		                  0u,
		                  0u,
		                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
		                 &os_err);

	    //OSStart(&os_err);
}


void enc_set_queue(OS_Q* osq)
{
	queue_setted = true;
	osQueue = osq;
}

void enc_set_sem(OS_SEM* sem_enc)
{
	sem_setted = true;
	osSem = sem_enc;
}
