/*
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2016-xx-xx
  * @brief   uCOS-III ϵͳ��ֲ
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32  ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************************
*/

/*
*********************************************************************************************************
*                                            �������ļ�
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

CPU_TS             ts_start;       //ʱ�������
CPU_TS             ts_end; 



/*
*********************************************************************************************************
*                                                 TCB
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;                                //������ƿ�

static  OS_TCB   AppTaskTmrTCB;


/*
*********************************************************************************************************
*                                                STACKS
*********************************************************************************************************
*/

static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];       //�����ջ

static  CPU_STK  AppTaskTmrStk [ APP_TASK_TMR_STK_SIZE ];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);                       //����������

static  void  AppTaskTmr  ( void * p_arg );


/*
*********************************************************************************************************
* ������ : main
* ����   : ��׼��C�������
* �β�   : ��
* ����ֵ : ��
* ע��   : 1) HAL��ʼ��:
*             a) ����FlashԤȡ��ָ������ݸ��ٻ��档
*             b) ����Systick�������жϡ�HAL_InitTick()�����Ѿ���ϵͳ��д��
*                ϵͳ���Լ���Systick��ʼ�������������ڶ���������֮���ʼ����
*********************************************************************************************************
*/
int main(void)
{
    OS_ERR   err;


    HAL_Init();                                                             //HAL��ʼ��,��ע�� 1

    BSP_SystemClkCfg();                                                     //��ʼ��CPUƵ��Ϊ 216Mhz

    CPU_Init();                                                             //��ʼ�� CPU �����ʱ��������ж�ʱ���������������

    Mem_Init();                                                             //��ʼ���ڴ������������ڴ�غ��ڴ�ر�

    CPU_IntDis();                                                           //��ֹ�����ж�

    OSInit(&err);                                                           //��ʼ��uC/OS-IIIϵͳ
    App_OS_SetAllHooks();

	/* ������ʼ���� */
    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB,                            //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Start",                            //��������
                 (OS_TASK_PTR ) AppTaskStart,                               //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_START_PRIO,                        //��������ȼ�
                 (CPU_STK    *)&AppTaskStartStk[0],                         //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE / 10,               //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,                    //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵOSCfg_TickRate_Hz/10��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������

    OSStart(&err);                                                          //�����������������uC/OS-III���ƣ�

}

/*
*********************************************************************************************************
* ������ ��AppTaskStart
* ����   : ����һ�����������ڶ�����ϵͳ�����󣬱����ʼ���δ������(�� BSP_Init ��ʵ��)��
* �β�   : p_arg   ��OSTaskCreate()�ڴ���������ʱ���ݹ������βΡ�
* ����ֵ : ��
* ע��   : 1) ��һ�д��� (void)p_arg; ��Ϊ�˷�ֹ������������Ϊ�β�p_arg��û���õ�
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR  err;


   (void)p_arg;

    BSP_Init();                                    //�弶��ʼ��

#if OS_CFG_STAT_TASK_EN > 0u                       //���ʹ�ܣ�Ĭ��ʹ�ܣ���ͳ������
    OSStatTaskCPUUsageInit(&err);                  //����û��Ӧ������ֻ�п�����������ʱ CPU �ģ����
#endif                                             //���������� OS_Stat_IdleCtrMax ��ֵ��Ϊ������� CPU
                                                   //ʹ����ʹ�ã���
#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();                   //��λ�����㣩��ǰ�����ж�ʱ��
#endif

		/* ���� AppTaskTmr ���� */
    OSTaskCreate((OS_TCB     *)&AppTaskTmrTCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Tmr",                             //��������
                 (OS_TASK_PTR ) AppTaskTmr,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_TMR_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskTmrStk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_TMR_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_TMR_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵOSCfg_TickRate_Hz/10��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������
								 
		OSTaskDel ( & AppTaskStartTCB, & err );                     //ɾ����ʼ������������������
		
		
}


/*
*********************************************************************************************************
*                                          TMR TASK
*********************************************************************************************************
*/
void TmrCallback (OS_TMR *p_tmr, void *p_arg) //�����ʱ��MyTmr�Ļص�����
{
	CPU_INT32U       cpu_clk_freq;	

  printf ( "%s", ( char * ) p_arg );
	
	cpu_clk_freq = BSP_ClkFreqGet(BSP_CLK_ID_SYSCLK);                   //��ȡCPUʱ�ӣ�ʱ������Ը�ʱ�Ӽ���
	
	LED1_TOGGLE; 
	
  ts_end = OS_TS_GET() - ts_start;     //��ȡ��ʱ���ʱ�������CPUʱ�ӽ��м�����һ������ֵ��
	                                     //�������㶨ʱʱ�䡣

	printf ( "\r\n��ʱ1s��ͨ��ʱ�����ö�ʱ %07d us���� %04d ms��\r\n", 
						ts_end / ( cpu_clk_freq / 1000000 ),     //����ʱʱ������� us 
						ts_end / ( cpu_clk_freq / 1000 ) );      //����ʱʱ������� ms 

	ts_start = OS_TS_GET();                            //��ȡ��ʱǰʱ���
	
}


static  void  AppTaskTmr ( void * p_arg )
{
	OS_ERR      err;
	OS_TMR      my_tmr;   //���������ʱ������

	
	(void)p_arg;


  /* ���������ʱ�� */
  OSTmrCreate ((OS_TMR              *)&my_tmr,             //�����ʱ������
               (CPU_CHAR            *)"MySoftTimer",       //���������ʱ��
               (OS_TICK              )10,                  //��ʱ����ʼֵ����10Hzʱ�����㣬��Ϊ1s
               (OS_TICK              )10,                  //��ʱ����������ֵ����10Hzʱ�����㣬��Ϊ1s
               (OS_OPT               )OS_OPT_TMR_PERIODIC, //�����Զ�ʱ
               (OS_TMR_CALLBACK_PTR  )TmrCallback,         //�ص�����
               (void                *)"Timer Over!",       //����ʵ�θ��ص�����
               (OS_ERR              *)err);                //���ش�������
							 
	/* ���������ʱ�� */						 
  OSTmrStart ((OS_TMR   *)&my_tmr, //�����ʱ������
              (OS_ERR   *)err);    //���ش�������
					 
	ts_start = OS_TS_GET();                       //��ȡ��ʱǰʱ���
							 
	while (DEF_TRUE) {                            //�����壬ͨ��д��һ����ѭ��

		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err ); //��������������

	}
	
}

