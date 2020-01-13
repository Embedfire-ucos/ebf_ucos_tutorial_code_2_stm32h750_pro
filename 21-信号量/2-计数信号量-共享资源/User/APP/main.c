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
#include "./key/bsp_key.h" 
#include "usart/bsp_usart.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

OS_SEM SemOfKey;          //信号量


/*
*********************************************************************************************************
*                                         任务控制块TCB
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;                                //任务控制块

static  OS_TCB   AppTaskKey1TCB;
static  OS_TCB   AppTaskKey2TCB;


/*
*********************************************************************************************************
*                                                STACKS
*********************************************************************************************************
*/

static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];       //任务堆栈

static  CPU_STK  AppTaskKey1Stk [ APP_TASK_KEY1_STK_SIZE ];
static  CPU_STK  AppTaskKey2Stk [ APP_TASK_KEY2_STK_SIZE ];


/*
*********************************************************************************************************
*                                            函数原型
*********************************************************************************************************
*/
static  void  AppTaskStart  (void *p_arg);                       //任务函数声明

static  void  AppTaskKey1  ( void * p_arg );
static  void  AppTaskKey2  ( void * p_arg );

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


		/* 创建多值信号量 SemOfKey */
    OSSemCreate((OS_SEM      *)&SemOfKey,    //指向信号量变量的指针
               (CPU_CHAR    *)"SemOfKey",    //信号量的名字
               (OS_SEM_CTR   )5,             //表示现有资源数目
               (OS_ERR      *)&err);         //错误类型
							 

		/* 创建 AppTaskKey1 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskKey1TCB,                             //任务控制块地址
                 (CPU_CHAR   *)"App Task Key1",                             //任务名称
                 (OS_TASK_PTR ) AppTaskKey1,                                //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_KEY1_PRIO,                         //任务的优先级
                 (CPU_STK    *)&AppTaskKey1Stk[0],                          //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_KEY1_STK_SIZE / 10,                //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_KEY1_STK_SIZE,                     //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型

		/* 创建 AppTaskKey2 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskKey2TCB,                             //任务控制块地址
                 (CPU_CHAR   *)"App Task Key2",                             //任务名称
                 (OS_TASK_PTR ) AppTaskKey2,                                //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_KEY2_PRIO,                         //任务的优先级
                 (CPU_STK    *)&AppTaskKey2Stk[0],                          //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_KEY2_STK_SIZE / 10,                //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_KEY2_STK_SIZE,                     //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型
    
		OSTaskDel ( & AppTaskStartTCB, & err );                     //删除起始任务本身，该任务不再运行
		
		
}


/*
*********************************************************************************************************
*                                          KEY1 TASK
*********************************************************************************************************
*/
static  void  AppTaskKey1 ( void * p_arg )
{
	OS_ERR      err;
	OS_SEM_CTR  ctr;
	
	(void)p_arg;

					 
	while (DEF_TRUE) {                                                         //任务体
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON ) //如果KEY1被按下
		{
			ctr = OSSemPend ((OS_SEM   *)&SemOfKey,               //等待该信号量 SemOfKey
								       (OS_TICK   )0,                       //下面选择不等待，该参无效
								       (OS_OPT    )OS_OPT_PEND_NON_BLOCKING,//如果没信号量可用不等待
								       (CPU_TS   *)0,                       //不获取时间戳
								       (OS_ERR   *)&err);                   //返回错误类型
			

			if ( err == OS_ERR_NONE )                      
				printf ( "\r\nKEY1被按下：成功申请到停车位，剩下%d个停车位。\r\n", ctr );
			else if ( err == OS_ERR_PEND_WOULD_BLOCK )
				printf ( "\r\nKEY1被按下：不好意思，现在停车场已满，请等待！\r\n" );
			


		}
		
		OSTimeDlyHMSM ( 0, 0, 0, 20, OS_OPT_TIME_DLY, & err );  //每20ms扫描一次
		
	}
	
}


/*
*********************************************************************************************************
*                                          KEY2 TASK
*********************************************************************************************************
*/
static  void  AppTaskKey2 ( void * p_arg )
{
	OS_ERR      err;
	OS_SEM_CTR  ctr;
	
	(void)p_arg;

					 
	while (DEF_TRUE) {                                                         //任务体
		if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON ) //如果KEY2被按下
		{
		  ctr = OSSemPost((OS_SEM  *)&SemOfKey,                                  //发布SemOfKey
							        (OS_OPT   )OS_OPT_POST_ALL,                            //发布给所有等待任务
							        (OS_ERR  *)&err);                                      //返回错误类型
                                                    //进入临界段
			
			printf ( "\r\nKEY2被按下：释放1个停车位，剩下%d个停车位。\r\n", ctr );
			
			
		}
		
		OSTimeDlyHMSM ( 0, 0, 0, 20, OS_OPT_TIME_DLY, & err );                    //每20ms扫描一次
		
	}
	
}

