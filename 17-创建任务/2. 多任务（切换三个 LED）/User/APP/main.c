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
  * ʵ��ƽ̨:Ұ��  STM32H750 ������  
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

static  OS_TCB       AppTaskStartTCB;
static  OS_TCB       AppTaskLed1TCB;
static  OS_TCB       AppTaskLed2TCB;
static  OS_TCB       AppTaskLed3TCB;
/*
*********************************************************************************************************
*                                            �����ջ
*********************************************************************************************************
*/
static  CPU_STK      AppTaskStartStk[APP_TASK_START_STK_SIZE];
static  CPU_STK      AppTaskLed1Stk[APP_TASK_LED1_STK_SIZE];
static  CPU_STK      AppTaskLed2Stk[APP_TASK_LED2_STK_SIZE];
static  CPU_STK      AppTaskLed3Stk[APP_TASK_LED3_STK_SIZE];
/*
*********************************************************************************************************
*                                            ����ԭ��
*********************************************************************************************************
*/

static  void  AppTaskStart (void  *p_arg);
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

    /* ����Led1���� */
    OSTaskCreate((OS_TCB     *)&AppTaskLed1TCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Led1",                             //��������
                 (OS_TASK_PTR ) AppTaskLed1,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_LED1_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskLed1Stk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_LED1_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_LED1_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵOSCfg_TickRate_Hz/10��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������


    /* ����Led2���� */
    OSTaskCreate((OS_TCB     *)&AppTaskLed2TCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Led2",                             //��������
                 (OS_TASK_PTR ) AppTaskLed2,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_LED2_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskLed2Stk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_LED2_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_LED2_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵOSCfg_TickRate_Hz/10��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������
								 
    /* ����Led3���� */
    OSTaskCreate((OS_TCB     *)&AppTaskLed3TCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Led3",                             //��������
                 (OS_TASK_PTR ) AppTaskLed3,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_LED3_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskLed3Stk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_LED3_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_LED3_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵOSCfg_TickRate_Hz/10��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������

		//OSTaskDel ( & AppTaskStartTCB, & err );                             //ɾ����ʼ����
		while(1)
		{
			OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );  //��ʱ1s
		}
		
}


/*
*********************************************************************************************************
*                                          LED1 ����
*********************************************************************************************************
*/

static  void  AppTaskLed1 ( void * p_arg )
{
    OS_ERR      err;


   (void)p_arg;                                      //û���õ��βΣ���ֹ����������
    
    LED_RGBOFF;                                      //�ر�RGB LED
    
    while (DEF_TRUE)                                 //�����壬ͨ����д��һ����ѭ��
    {                                               
        LED1_TOGGLE;                                 //LED1��˸
        OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );  //��ʱ1s
    }
		
		
}


/*
*********************************************************************************************************
*                                          LED2 ����
*********************************************************************************************************
*/

static  void  AppTaskLed2 ( void * p_arg )
{
    OS_ERR      err;


   (void)p_arg;                                      //û���õ��βΣ���ֹ����������
    
    LED_RGBOFF;                                      //�ر�RGB LED
    
    while (DEF_TRUE)                                 //�����壬ͨ����д��һ����ѭ��
    {                                               
        LED2_TOGGLE;                                 //LED2��˸
        OSTimeDly ( 5000, OS_OPT_TIME_DLY, & err );  //��ʱ5s
    }
		
		
}

/*
*********************************************************************************************************
*                                          LED3 ����
*********************************************************************************************************
*/

static  void  AppTaskLed3 ( void * p_arg )
{
    OS_ERR      err;


   (void)p_arg;                                      //û���õ��βΣ���ֹ����������
    
    LED_RGBOFF;                                      //�ر�RGB LED
    
    while (DEF_TRUE)                                 //�����壬ͨ����д��һ����ѭ��
    {                                               
        LED3_TOGGLE;                                 //LED3��˸
        OSTimeDly ( 10000, OS_OPT_TIME_DLY, & err ); //��ʱ10s
    }
		
		
}


