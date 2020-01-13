/*
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2016-xx-xx
  * @brief   uCOS-III 系统移植
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火  STM32  开发板  
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************************
*/

/*
*********************************************************************************************************
*                                            包含的文件
*********************************************************************************************************
*/
#include  "os.h"
#include  <stdio.h>
#include  <bsp.h>
#include  <app_cfg.h>
#include  <os_app_hooks.h>
#include  <stm32h7xx_hal.h>
#include "./led/bsp_led.h" 
#include "usart/bsp_usart.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

CPU_TS             ts_start;       //时间戳变量
CPU_TS             ts_end; 



/*
*********************************************************************************************************
*                                                 TCB
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;                                //任务控制块

static  OS_TCB   AppTaskTmrTCB;


/*
*********************************************************************************************************
*                                                STACKS
*********************************************************************************************************
*/

static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];       //任务堆栈

static  CPU_STK  AppTaskTmrStk [ APP_TASK_TMR_STK_SIZE ];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);                       //任务函数声明

static  void  AppTaskTmr  ( void * p_arg );


/*
*********************************************************************************************************
* 函数名 : main
* 描述   : 标准的C函数入口
* 形参   : 无
* 返回值 : 无
* 注意   : 1) HAL初始化:
*             a) 配置Flash预取，指令和数据高速缓存。
*             b) 配置Systick以生成中断。HAL_InitTick()函数已经被系统重写，
*                系统有自己的Systick初始化函数，建议在多任务启动之后初始化。
*********************************************************************************************************
*/
int main(void)
{
    OS_ERR   err;


    HAL_Init();                                                             //HAL初始化,见注意 1

    BSP_SystemClkCfg();                                                     //初始化CPU频率为 216Mhz

    CPU_Init();                                                             //初始化 CPU 组件（时间戳、关中断时间测量和主机名）

    Mem_Init();                                                             //初始化内存管理组件（堆内存池和内存池表）

    CPU_IntDis();                                                           //禁止所有中断

    OSInit(&err);                                                           //初始化uC/OS-III系统
    App_OS_SetAllHooks();

	/* 创建起始任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB,                            //任务控制块地址
                 (CPU_CHAR   *)"App Task Start",                            //任务名称
                 (OS_TASK_PTR ) AppTaskStart,                               //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_START_PRIO,                        //任务的优先级
                 (CPU_STK    *)&AppTaskStartStk[0],                         //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE / 10,               //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,                    //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型

    OSStart(&err);                                                          //启动多任务管理（交由uC/OS-III控制）

}

/*
*********************************************************************************************************
* 函数名 ：AppTaskStart
* 描述   : 这是一个启动任务，在多任务系统启动后，必须初始化滴答计数器(在 BSP_Init 中实现)。
* 形参   : p_arg   是OSTaskCreate()在创建该任务时传递过来的形参。
* 返回值 : 无
* 注意   : 1) 第一行代码 (void)p_arg; 是为了防止编译器报错，因为形参p_arg并没有用到
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR  err;


   (void)p_arg;

    BSP_Init();                                    //板级初始化

#if OS_CFG_STAT_TASK_EN > 0u                       //如果使能（默认使能）了统计任务
    OSStatTaskCPUUsageInit(&err);                  //计算没有应用任务（只有空闲任务）运行时 CPU 的（最大）
#endif                                             //容量（决定 OS_Stat_IdleCtrMax 的值，为后面计算 CPU
                                                   //使用率使用）。
#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();                   //复位（清零）当前最大关中断时间
#endif

		/* 创建 AppTaskTmr 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskTmrTCB,                             //任务控制块地址
                 (CPU_CHAR   *)"App Task Tmr",                             //任务名称
                 (OS_TASK_PTR ) AppTaskTmr,                                //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_TMR_PRIO,                         //任务的优先级
                 (CPU_STK    *)&AppTaskTmrStk[0],                          //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_TMR_STK_SIZE / 10,                //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_TMR_STK_SIZE,                     //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型
								 
		OSTaskDel ( & AppTaskStartTCB, & err );                     //删除起始任务本身，该任务不再运行
		
		
}


/*
*********************************************************************************************************
*                                          TMR TASK
*********************************************************************************************************
*/
void TmrCallback (OS_TMR *p_tmr, void *p_arg) //软件定时器MyTmr的回调函数
{
	CPU_INT32U       cpu_clk_freq;	

  printf ( "%s", ( char * ) p_arg );
	
	cpu_clk_freq = BSP_ClkFreqGet(BSP_CLK_ID_SYSCLK);                   //获取CPU时钟，时间戳是以该时钟计数
	
	LED1_TOGGLE; 
	
  ts_end = OS_TS_GET() - ts_start;     //获取定时后的时间戳（以CPU时钟进行计数的一个计数值）
	                                     //，并计算定时时间。

	printf ( "\r\n定时1s，通过时间戳测得定时 %07d us，即 %04d ms。\r\n", 
						ts_end / ( cpu_clk_freq / 1000000 ),     //将定时时间折算成 us 
						ts_end / ( cpu_clk_freq / 1000 ) );      //将定时时间折算成 ms 

	ts_start = OS_TS_GET();                            //获取定时前时间戳
	
}


static  void  AppTaskTmr ( void * p_arg )
{
	OS_ERR      err;
	OS_TMR      my_tmr;   //声明软件定时器对象

	
	(void)p_arg;


  /* 创建软件定时器 */
  OSTmrCreate ((OS_TMR              *)&my_tmr,             //软件定时器对象
               (CPU_CHAR            *)"MySoftTimer",       //命名软件定时器
               (OS_TICK              )10,                  //定时器初始值，依10Hz时基计算，即为1s
               (OS_TICK              )10,                  //定时器周期重载值，依10Hz时基计算，即为1s
               (OS_OPT               )OS_OPT_TMR_PERIODIC, //周期性定时
               (OS_TMR_CALLBACK_PTR  )TmrCallback,         //回调函数
               (void                *)"Timer Over!",       //传递实参给回调函数
               (OS_ERR              *)err);                //返回错误类型
							 
	/* 启动软件定时器 */						 
  OSTmrStart ((OS_TMR   *)&my_tmr, //软件定时器对象
              (OS_ERR   *)err);    //返回错误类型
					 
	ts_start = OS_TS_GET();                       //获取定时前时间戳
							 
	while (DEF_TRUE) {                            //任务体，通常写成一个死循环

		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err ); //不断阻塞该任务

	}
	
}

