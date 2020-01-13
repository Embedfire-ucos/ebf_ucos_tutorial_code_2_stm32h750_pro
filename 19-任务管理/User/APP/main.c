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
  * ʵ��ƽ̨:����  STM32 h767 ������  
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
#include  <stdio.h>
#include  <bsp.h>
#include  <app_cfg.h>
#include  <os_app_hooks.h>
#include  <stm32h7xx_hal.h>
#include "./led/bsp_led.h" 

/*
*********************************************************************************************************
*                                         ������ƿ�TCB
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;    //������ƿ�

static  OS_TCB   AppTaskLed1TCB;
static  OS_TCB   AppTaskLed2TCB;
static  OS_TCB   AppTaskLed3TCB;


/*
*********************************************************************************************************
*                                                STACKS
*********************************************************************************************************
*/

static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];       //�����ջ

static  CPU_STK  AppTaskLed1Stk [ APP_TASK_LED1_STK_SIZE ];
static  CPU_STK  AppTaskLed2Stk [ APP_TASK_LED2_STK_SIZE ];
static  CPU_STK  AppTaskLed3Stk [ APP_TASK_LED3_STK_SIZE ];


/*
*********************************************************************************************************
*                                            ����ԭ��
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);               //����������

static  void  AppTaskLed1  ( void * p_arg );
static  void  AppTaskLed2  ( void * p_arg );
static  void  AppTaskLed3  ( void * p_arg );

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

		/* ���� LED1 ���� */
    OSTaskCreate((OS_TCB     *)&AppTaskLed1TCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Led1",                             //��������
                 (OS_TASK_PTR ) AppTaskLed1,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_LED1_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskLed1Stk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_LED1_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_LED1_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵ��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������
		
		/* ���� LED2 ���� */
    OSTaskCreate((OS_TCB     *)&AppTaskLed2TCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Led2",                             //��������
                 (OS_TASK_PTR ) AppTaskLed2,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_LED2_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskLed2Stk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_LED2_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_LED2_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵ��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������								 

		/* ���� LED3 ���� */
    OSTaskCreate((OS_TCB     *)&AppTaskLed3TCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Led3",                             //��������
                 (OS_TASK_PTR ) AppTaskLed3,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_LED3_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskLed3Stk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_LED3_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_LED3_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵ��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������

    
		OSTaskDel ( 0, & err );                     //ɾ����ʼ������������������
		
		
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


	while (DEF_TRUE) {                                   //�����壬ͨ��д��һ����ѭ��
		LED1_TOGGLE;                                 //�л� LED1 ������״̬
		
		value = OSTaskRegGet ( 0, 0, & err );              //��ȡ��������Ĵ���ֵ
		
		if ( value < 10 )                                  //�������Ĵ���ֵ<10
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );          //�����ۼ�����Ĵ���ֵ
		}		
		else                                               //����ۼӵ�10
		{
			OSTaskRegSet ( 0, 0, 0, & err );                 //������Ĵ���ֵ��0
			printf("�ָ�LED2����\n");
			OSTaskResume ( & AppTaskLed2TCB, & err );        //�ָ� LED2 ����
     
			printf("�ָ�LED3����\n");
			OSTaskResume ( & AppTaskLed3TCB, & err );        //�ָ� LED3 ����
			
		}
		
		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );        //�������ʱ1000��ʱ�ӽ��ģ�1s��
		
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


	while (DEF_TRUE) {                                 //�����壬ͨ��д��һ����ѭ��
		LED2_TOGGLE;                               //�л� LED2 ������״̬
		
		value = OSTaskRegGet ( 0, 0, & err );            //��ȡ��������Ĵ���ֵ
		
		if ( value < 5 )                                 //�������Ĵ���ֵ<5
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );        //�����ۼ�����Ĵ���ֵ
		}
		else                                             //����ۼӵ�5
		{
			OSTaskRegSet ( 0, 0, 0, & err );               //������Ĵ���ֵ��0
			printf("����LED2����������\n");
			OSTaskSuspend ( 0, & err );                    //��������
			
		}
		
		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );      //�������ʱ1000��ʱ�ӽ��ģ�1s��
		
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


	while (DEF_TRUE) {                                 //�����壬ͨ��д��һ����ѭ��
		LED3_TOGGLE;                               //�л� LED3 ������״̬
		
		value = OSTaskRegGet ( 0, 0, & err );            //��ȡ��������Ĵ���ֵ
		
		if ( value < 5 )                                 //�������Ĵ���ֵ<5
		{
			OSTaskRegSet ( 0, 0, ++ value, & err );        //�����ۼ�����Ĵ���ֵ
		}
		else                                             //����ۼӵ�5
		{
			OSTaskRegSet ( 0, 0, 0, & err );               //������Ĵ���ֵ����
			printf("����LED3����������\n");
			OSTaskSuspend ( 0, & err );                    //��������
		}

		OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );      //�������ʱ1000��ʱ�ӽ��ģ�1s��
		
	}

		
}
