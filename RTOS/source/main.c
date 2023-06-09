#include "hardware.h"
#include  <os.h>

#include <stdlib.h>

/* LEDs */
#define LED_R_PORT            PORTB
#define LED_R_GPIO            GPIOB
#define LED_G_PORT            PORTE
#define LED_G_GPIO            GPIOE
#define LED_B_PORT            PORTB
#define LED_B_GPIO            GPIOB
#define LED_R_PIN             22
#define LED_G_PIN             26
#define LED_B_PIN             21
#define LED_B_ON()           (LED_B_GPIO->PCOR |= (1 << LED_B_PIN))
#define LED_B_OFF()          (LED_B_GPIO->PSOR |= (1 << LED_B_PIN))
#define LED_B_TOGGLE()       (LED_B_GPIO->PTOR |= (1 << LED_B_PIN))
#define LED_G_ON()           (LED_G_GPIO->PCOR |= (1 << LED_G_PIN))
#define LED_G_OFF()          (LED_G_GPIO->PSOR |= (1 << LED_G_PIN))
#define LED_G_TOGGLE()       (LED_G_GPIO->PTOR |= (1 << LED_G_PIN))
#define LED_R_ON()           (LED_R_GPIO->PCOR |= (1 << LED_R_PIN))
#define LED_R_OFF()          (LED_R_GPIO->PSOR |= (1 << LED_R_PIN))
#define LED_R_TOGGLE()       (LED_R_GPIO->PTOR |= (1 << LED_R_PIN))

/* Task Start */
#define TASKSTART_STK_SIZE 		512u //stack size.
#define TASKSTART_PRIO 			2u //priority.
static OS_TCB TaskStartTCB;
static CPU_STK TaskStartStk[TASKSTART_STK_SIZE]; //reservs CPU stack.

/* Task 2 */
#define TASK2_STK_SIZE			256u //stack size.
#define TASK2_STK_SIZE_LIMIT	(TASK2_STK_SIZE / 10u)
#define TASK2_PRIO              3u //priority.
static OS_TCB Task2TCB; //task information handled by the OS.
static CPU_STK Task2Stk[TASK2_STK_SIZE];

/* Task 3 */
#define TASK3_STK_SIZE			256u //stack size.
#define TASK3_STK_SIZE_LIMIT	(TASK2_STK_SIZE / 10u)
#define TASK3_PRIO              3u //priority.
static OS_TCB Task3TCB; //task information handled by the OS.
static CPU_STK Task3Stk[TASK2_STK_SIZE];

/* Example semaphore */
static OS_SEM semTest; //SEMASPHORE
//QUEUE
static OS_Q osqueue; //QUEUE

static void Task2(void *p_arg) {
	char msg[] = "blue";
    (void)p_arg; //arg pointer.
    OS_ERR os_err; //in cause there are errors in the CPU.

    while (1) {
        OSSemPost(&semTest, OS_OPT_POST_1, &os_err); //set semasphore.
        OSQPost(&osqueue, &msg, sizeof(msg), OS_OPT_POST_ALL, &os_err);
        OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_STRICT, &os_err);
        //LED_R_TOGGLE(); //after all of the CPU functions, comes the real task of task2.
    }
}

static void Task3(void *p_arg) {
	uint8_t msgsize;
	char *msg;
    (void)p_arg; //arg pointer.
    OS_ERR os_err; //in cause there are errors in the CPU.

    while (1) {
        //OSTimeDly(100, OS_OPT_TIME_DLY, &os_err); //100 ticks.
//        OSSemPend(&semTest, 0, OS_OPT_PEND_BLOCKING, 0, &os_err); //if semasphore is set, clear.
//        LED_B_TOGGLE(); //after all of the CPU functions, comes the real task of task2.
        msg = (char*)OSQPend(&osqueue, 0, OS_OPT_PEND_BLOCKING, &msgsize, 0, &os_err);
        if(msg[1] == 'e') //red
        	LED_R_TOGGLE();
        if(msg[1] == 'r') //green
            LED_G_TOGGLE();
        if(msg[1] == 'l') //blue
            LED_B_TOGGLE();

    }
}


static void TaskStart(void *p_arg) {
    (void)p_arg;
    OS_ERR os_err; //in cause there are errors in the CPU.

    /* Initialize the uC/CPU Services. */
    CPU_Init();

#if OS_CFG_STAT_TASK_EN > 0u
    /* (optional) Compute CPU capacity with no task running */
    OSStatTaskCPUUsageInit(&os_err); //to check the status of the CPU.
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

    /* Create semaphore */
    OSSemCreate(&semTest, "Sem Test", 0u, &os_err);
    //create queue
    OSQCreate(&osqueue, "OS queue", 5, &os_err);

    /* Create Task2 */
    OSTaskCreate(&Task2TCB, 			//tcb
                 "Task 2",				//name
                  Task2,				//func
                  0u,					//arg
                  TASK2_PRIO,			//prio
                 &Task2Stk[0u],			//stack (stack pointer)
                  TASK2_STK_SIZE_LIMIT,	//stack limit
                  TASK2_STK_SIZE,		//stack size
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &os_err);

    /* Create Task3 */
        OSTaskCreate(&Task3TCB, 			//tcb
                     "Task 3",				//name
                      Task3,				//func
                      0u,					//arg
                      TASK3_PRIO,			//prio
                     &Task3Stk[0u],			//stack (stack pointer)
                      TASK3_STK_SIZE_LIMIT,	//stack limit
                      TASK3_STK_SIZE,		//stack size
                      0u,
                      0u,
                      0u,
                     (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     &os_err);

    while (1) {
        OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err); //starts a timer of the OS and the CPU will concentrate in other task.
        																	//and when the time is out, the CPU returns to this task.
        LED_G_TOGGLE();
    }
}

int main(void) {
    OS_ERR err;

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR  cpu_err;
#endif

    hw_Init();

    /* RGB LED */
    SIM->SCGC5 |= (SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK);
    LED_B_PORT->PCR[LED_B_PIN] = PORT_PCR_MUX(1);
    LED_G_PORT->PCR[LED_G_PIN] = PORT_PCR_MUX(1);
    LED_R_PORT->PCR[LED_R_PIN] = PORT_PCR_MUX(1);
    LED_B_GPIO->PDDR |= (1 << LED_B_PIN);
    LED_G_GPIO->PDDR |= (1 << LED_G_PIN);
    LED_R_GPIO->PDDR |= (1 << LED_R_PIN);
    LED_B_GPIO->PSOR |= (1 << LED_B_PIN);
    LED_G_GPIO->PSOR |= (1 << LED_G_PIN);
    LED_R_GPIO->PSOR |= (1 << LED_R_PIN);

    OSInit(&err);
 #if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	 /* Enable task round robin. */
	 OSSchedRoundRobinCfg((CPU_BOOLEAN)1, 0, &err);
 #endif
    OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz); //configures the tick freq of the RTOS. (100MHz/1K = 100KHz)

    OSTaskCreate(&TaskStartTCB,
                 "App Task Start",
                  TaskStart,
                  0u,
                  TASKSTART_PRIO,
                 &TaskStartStk[0u],
                 (TASKSTART_STK_SIZE / 10u),
                  TASKSTART_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),
                 &err);

    OSStart(&err); //starts the OS by leaving this function and NEVER cames back.

	/* Should Never Get Here */
    while (1) {

    }
}
