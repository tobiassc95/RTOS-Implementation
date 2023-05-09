#include "display.h"
#include "portpin.h"
#include "os.h"

//DEFINIMOS LOS CARACTERES PARA EL SEVEN SEGMENT DISPLAY.
//SIENDO EL MSb = A y EL LSb = G.
#define _0 0x7E
#define _1 0x30
#define _2 0x6D
#define _3 0x79
#define _4 0x33
#define _5 0x5B
#define _6 0x5F
#define _7 0x70
#define _8 0x7F
#define _9 0x7B
#define _A 0x77
#define _a 0x7D
#define _b 0x1F
#define _C 0x4E
#define _c 0x0D
#define _d 0x3D
#define _E 0x4F
#define _e 0x6F
#define _F 0x47
#define _G 0x5E
#define _g _9
#define _H 0x37
#define _h 0x17
#define _I 0x06
#define _i 0x04
#define _l _1
#define _J 0x3C
#define _j _J
#define _L 0x0E
#define _n 0x15
#define _O _0
#define _o 0x1D
#define _P 0x67
#define _p _P
#define _q 0x73
#define _r 0x05
#define _S _5
#define _s _S
#define _t 0x0F
#define _U 0x3E
#define _u 0x1C
#define _Y 0x3B
#define _y _Y
#define HYPHEN 0x01 //GUION.
#define EMPTY 0x00

//Definimos los pines que usaremos para el display.
#define PTC5 PORTNUM2PIN(PORT_C, 5)
#define PTC7 PORTNUM2PIN(PORT_C, 7)
#define PTC0 PORTNUM2PIN(PORT_C, 0)
#define PTC9 PORTNUM2PIN(PORT_C, 9)
#define PTC8 PORTNUM2PIN(PORT_C, 8)
#define PTC1 PORTNUM2PIN(PORT_C, 1)
#define PTB19 PORTNUM2PIN(PORT_B, 19)
#define PTB18 PORTNUM2PIN(PORT_B, 18)
#define PTC17 PORTNUM2PIN(PORT_C, 17)
#define PTC16 PORTNUM2PIN(PORT_C, 16)

//display task
#define DISPTASK_STKSIZ	256u //stack size.
#define DISPTASK_STKSIZ_LIMIT (DISPTASK_STKSIZ/10)
#define DISPTASK_PRIOR 2u //priority.

//display task
static OS_TCB disptask_tcb; //task information handled by the OS.
static CPU_STK disptask_stk[DISPTASK_STKSIZ];

//LOOK UP TABLE. los numeros del display.
static const uint8_t displaynumb[10] = {_0, _1, _2, _3, _4, _5, _6, _7, _8, _9};
//LOOK UP TABLE. los caracteres del display.
static const uint8_t displaychar[52] = {_A, EMPTY, _C, EMPTY, _E, _F, _G, _H, _I, _J, EMPTY, _L, EMPTY, EMPTY, _O, _P, EMPTY, EMPTY, _S, EMPTY, _U, EMPTY, EMPTY, EMPTY, _Y, EMPTY,
										_a, _b, _c, _d, _e, EMPTY, _g, _h, _i, _j, EMPTY, _l,EMPTY, _n, _o, _p, _q, _r, _s, _t, _u, EMPTY, EMPTY, EMPTY, _y, EMPTY};
//los pines que corresponden a los segmentos ABCDEFG.
static const uint8_t pinsegment[7] = {PTB18, PTB19, PTC1, PTC8, PTC9, PTC0, PTC7};
//el valor que se mostrará en cada display.
static uint8_t displayval[4];
//el punto de los displays.
static bool displaydot[4];


//para el systick (PISR).
static void displayDraw (void *p_arg);

void displayConfiguration (void) {
	OS_ERR oserr;
	uint8_t i;

	//Conectamos todos lo pines a la gpio.
	for (i = 0; i < 7; i++) {
		PINconfigure(pinsegment[i], PIN_MUX1, PIN_IRQ_DISABLE);
	}
	PINconfigure(PTC5, PIN_MUX1, PIN_IRQ_DISABLE); //pin DP.
	PINconfigure(PTC17, PIN_MUX1, PIN_IRQ_DISABLE);
	PINconfigure(PTC16, PIN_MUX1, PIN_IRQ_DISABLE);

	//DEFINIMOS LOS PINES DE ENTRADA Y SALIDA.
	for (i = 0; i < 7; i++) {
		PINmode(pinsegment[i], PIN_OUTPUT);
	}
	PINmode(PTC5, PIN_OUTPUT); //pinDP.
	PINmode(PTC17, PIN_OUTPUT);
	PINmode(PTC16, PIN_OUTPUT);

	OSTaskCreate(&disptask_tcb,"display task",displayDraw,0,DISPTASK_PRIOR,&disptask_stk[0],(DISPTASK_STKSIZ/10),DISPTASK_STKSIZ,0,0,0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),&oserr);

	//Limpiamos los displays.
	displayCleanAll();
}

void displayWriteDig (uint8_t dig, uint8_t value) {
	if (value >= 0 && value <= 9)
		displayval[dig] = displaynumb[value];
	else
		displayval[dig] = EMPTY;
}

void displayWriteChar (uint8_t dig, char value) {
	if(value >= '0' && value <= '9') //estamos en numeros.
		displayval[dig] = displaynumb[value - '0'];
	else if(value >= 'A' && value <= 'Z') //estamos en mayusculas.
		displayval[dig] = displaychar[value - 'A'];
	else if (value >= 'a' && value <= 'z') //estamos en minusculas.
		displayval[dig] = displaychar[value - 'a' + 26];
	else if(value == '-')
		displayval[dig] = HYPHEN;
	else
		displayval[dig] = EMPTY;
}

void displayWriteNumb (uint16_t value) {
	uint8_t i, j;
	uint16_t temp;

	if(value >= 0 && value <= 9999) {
		for (i = 0; i < 4; i++) {
			temp = value;
			for (j = 0; j < 3-i; j++) {
				temp /= 10;
				if (temp == 0)
					break;
			}
			if (temp == 0 && i < 3)
				displayCleanDig(i);
			else
				displayWriteDig(i, temp % 10);
		}
	}
	else {
		displayCleanAll();
	}
}

void displayWriteStr (char* value) {
	uint8_t i;
	bool endstr = false;

	for (i = 0; i < 4; i++) {
		if(*(value+i) == '\0' && !endstr)
			endstr = true;
		if(endstr)
			displayCleanDig(i);
		else
			displayWriteChar(i, *(value+i));
	}
}

void displayWriteDot(uint8_t dig) {
	displaydot[dig] = true;
}

void displayCleanDig (uint8_t dig) {
	displayval[dig] = EMPTY;
}

void displayCleanDot(uint8_t dig) {
	displaydot[dig] = false;
}

void displayCleanAll (void) {
	uint8_t i;

	for(i = 0; i < 4; i++) {
		displayCleanDig(i);
		displayCleanDot(i);
	}
}

void displayDraw (void *p_arg) { //para el systick
	//para la direccion del encoder que prende los displays.
	OS_ERR oserr;
	static uint8_t dig = 0; //current digit.
	uint8_t i;
	(void)p_arg;

	while (1) {
		PINwrite(PTC16, dig & 1); //sel0
		PINwrite(PTC17, (dig & 1<<1)>>1); //sel1

		for(i = 0; i < 7; i++) {
			PINwrite(pinsegment[i], (displayval[dig] & 1<<(6 - i))>>(6 - i));
		}
		PINwrite(PTC5, displaydot[dig]); //sel0

		dig++;
		if(dig == 4)
			dig = 0;
		OSTimeDly(5, OS_OPT_TIME_DLY, &oserr); //5 ticks = 5msec
	}
}
