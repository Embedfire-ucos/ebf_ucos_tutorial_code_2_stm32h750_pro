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
  * 实验平台:秉火  STM32 h767 开发板  
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
#include  <stdio.h>
#include  <bsp.h>
#include  <app_cfg.h>
#include  <os_app_hooks.h>
#include  <stm32h7xx_hal.h>
#include "./led/bsp_led.h" 

/*
*********************************************************************************************************
*                                         任务控制块TCB
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;    //任务控制块

static  OS_TCB   AppTaskLed1TCB;
static  OS_TCB   AppTaskLed2TCB;
static  OS_TCB   AppTaskLed3TCB;


/*
*********************************************************************************************************
*                                                STACKS
*********************************************************************************************************
*/

static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];       //任务堆栈

static  CPU_STK  AppTaskLed1Stk [ APP_TASK_LED1_STK_SIZE ];
static  CPU_STK  AppTaskLed2Stk [ APP_TASK_LED2_STK_SIZE ];
static  CPU_STK  AppTaskLed3Stk [ APP_TASK_LED3_STK_SIZE ];


/*
*********************************************************************************************************
*                                            函数原型
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);               //任务函数声明

static  void  AppTaskLed1  ( void * p_arg );
static  void  AppTaskLed2  ( void * p_arg );
static  void  AppTaskLed3  ( void * p_arg );

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

		/* 创建 LED1 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskLed1TCB,                             //任务控制块地址
                 (CPU_CHAR   *)"App Task Led1",                             //任务名称
                 (OS_TASK_PTR ) AppTaskLed1,                                //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_LED1_PRIO,                         //任务的优先级
                 (CPU_STK    *)&AppTaskLed1Stk[0],                          //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_LED1_STK_SIZE / 10,                //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_LED1_STK_SIZE,                     //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型
		
		/* 创建 LED2 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskLed2TCB,                             //任务控制块地址
                 (CPU_CHAR   *)"App Task Led2",                             //任务名称
                 (OS_TASK_PTR ) AppTaskLed2,                                //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_LED2_PRIO,                         //任务的优先级
                 (CPU_STK    *)&AppTaskLed2Stk[0],                          //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_LED2_STK_SIZE / 10,                //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_LED2_STK_SIZE,                     //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型								 

		/* 创建 LED3 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskLed3TCB,                             //任务控制块地址
                 (CPU_CHAR   *)"App Task Led3",                             //任务名称
                 (OS_TASK_PTR ) AppTaskLed3,                                //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_LED3_PRIO,                         //任务的优先级
                 (CPU_STK    *)&AppTaskLed3Stk[0],                          //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_LED3_STK_SIZE / 10,                //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_LED3_STK_SIZE,                     //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型

    
		OSTaskDel ( 0, & err );                     //删除起始任务本身，该任务不再运行
		
		
}


/*
*********************************************************************************************************
*                                          LED1 TASK
*********************************************************************************************************
*/

static  void  AppTaskLed1 ( void * p_arg )
{
	OS_ERR      err;
	OS_REG      value;	

	
	(void)p_arg;


	while (DEF_TRUE) {                                   //任务体，通常写成一个死循环
		LED1_TOGGLE;                                 //切换 LED1 的亮灭状态
		
		value = OSTaskRegGet ( 0, 0, & err );              //获取自身任务寄存器值
		
		if ( value < 10 )                                  //如果任务寄存器值<10
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );          //继续累加任务寄存器值
		}		
		else                                               //如果累加到10
		{
			OSTaskRegSet ( 0, 0, 0, & err );                 //将任务寄存器值归0
			printf("恢复LED2任务！\n");
			OSTaskResume ( & AppTaskLed2TCB, & err );        //恢复 LED2 任务
     
			printf("恢复LED3任务！\n");
			OSTaskResume ( & AppTaskLed3TCB, & err );        //恢复 LED3 任务
			
		}
		
		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );        //相对性延时1000个时钟节拍（1s）
		
	}
		
		
}


/*
*********************************************************************************************************
*                                          LED2 TASK
*********************************************************************************************************
*/

static  void  AppTaskLed2 ( void * p_arg )
{
	OS_ERR      err;
	OS_REG      value;


	(void)p_arg;


	while (DEF_TRUE) {                                 //任务体，通常写成一个死循环
		LED2_TOGGLE;                               //切换 LED2 的亮灭状态
		
		value = OSTaskRegGet ( 0, 0, & err );            //获取自身任务寄存器值
		
		if ( value < 5 )                                 //如果任务寄存器值<5
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );        //继续累加任务寄存器值
		}
		else                                             //如果累加到5
		{
			OSTaskRegSet ( 0, 0, 0, & err );               //将任务寄存器值归0
			printf("挂起LED2任务（自身）！\n");
			OSTaskSuspend ( 0, & err );                    //挂起自身
			
		}
		
		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );      //相对性延时1000个时钟节拍（1s）
		
	}
		
		
}


/*
*********************************************************************************************************
*                                          LED3 TASK
*********************************************************************************************************
*/

static  void  AppTaskLed3 ( void * p_arg )
{
	OS_ERR      err;
	OS_REG      value;


	(void)p_arg;


	while (DEF_TRUE) {                                 //任务体，通常写成一个死循环
		LED3_TOGGLE;                               //切换 LED3 的亮灭状态
		
		value = OSTaskRegGet ( 0, 0, & err );            //获取自身任务寄存器值
		
		if ( value < 5 )                                 //如果任务寄存器值<5
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );        //继续累加任务寄存器值
		}
		else                                             //如果累加到5
		{
			OSTaskRegSet ( 0, 0, 0, & err );               //将任务寄存器值归零
			printf("挂起LED3任务（自身）！\n");
			OSTaskSuspend ( 0, & err );                    //挂起自身
		}

		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );      //相对性延时1000个时钟节拍（1s）
		
	}

		
}
