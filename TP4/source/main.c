#include "hardware.h"
#include "os.h"

//app task
#define APPTASK_STKSIZ	256u //stack size.
#define APPTASK_STKSIZ_LIMIT (APPTASK_STKSIZ/10) //watermark limit.
#define APPTASK_PRIOR 3u //priority.

//app task
static OS_TCB apptask_tcb; //task information handled by the OS.
static CPU_STK apptask_stk[APPTASK_STKSIZ];

void main (void);
void appinit(void);
void apptask (void *p_arg);

void main (void) {
	OS_ERR oserr; //to check error OS status.

	#if (CPU_CFG_NAME_EN == DEF_ENABLED)
	CPU_ERR  cpu_err;
	#endif

    hw_Init(); //hardware init -> inicializa TODO (CLKs, etc);
    OSInit(&oserr);

    #if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
   	OSSchedRoundRobinCfg((CPU_BOOLEAN)1, 0, &oserr); //Enable task round robin.
    #endif
    OS_CPU_SysTickInit(SystemCoreClock / (uint32_t)OSCfg_TickRate_Hz); //configures the tick freq of the RTOS. (100MHz/1K = 100KHz)

    OSTaskCreate(&apptask_tcb,"app task",apptask,0,APPTASK_PRIOR,&apptask_stk[0],(APPTASK_STKSIZ/10),APPTASK_STKSIZ,0,0,0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR | OS_OPT_TASK_SAVE_FP),&oserr);

    //hw_DisableInterrupts(); //deshabilita las interrupciones para poder configurar todo lo que se necesita en app_init();
    appinit();
    //hw_EnableInterrupts(); //y las vuelve a inicializar.

    OSStart(&oserr); //the OS has the control.

//    __FOREVER__
//        App_Run(); /* Program-specific loop  */
}
