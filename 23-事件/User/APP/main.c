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
#include "./key/bsp_key.h" 
#include "usart/bsp_usart.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

OS_FLAG_GRP flag_grp;                   //�����¼���־��

#define KEY1_EVENT  (0x01 << 0)//�����¼������λ0
#define KEY2_EVENT  (0x01 << 1)//�����¼������λ1

/*
*********************************************************************************************************
*                                                 TCB
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;      //������ƿ�

static  OS_TCB   AppTaskPostTCB;
static  OS_TCB   AppTaskPendTCB;


/*
*********************************************************************************************************
*                                                STACKS
*********************************************************************************************************
*/

static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];       //�����ջ

static  CPU_STK  AppTaskPostStk [ APP_TASK_POST_STK_SIZE ];
static  CPU_STK  AppTaskPendStk [ APP_TASK_PEND_STK_SIZE ];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);               //����������

static  void  AppTaskPost   ( void * p_arg );
static  void  AppTaskPend   ( void * p_arg );


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

  /* �����¼���־�� flag_grp */
  OSFlagCreate ((OS_FLAG_GRP  *)&flag_grp,        //ָ���¼���־���ָ��
                (CPU_CHAR     *)"FLAG For Test",  //�¼���־�������
                (OS_FLAGS      )0,                //�¼���־��ĳ�ʼֵ
                (OS_ERR       *)&err);					  //���ش�������
							  

		/* ���� AppTaskPost ���� */
    OSTaskCreate((OS_TCB     *)&AppTaskPostTCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Post",                             //��������
                 (OS_TASK_PTR ) AppTaskPost,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_POST_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskPostStk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_POST_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_POST_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵOSCfg_TickRate_Hz/10��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������

		/* ���� AppTaskPend ���� */
    OSTaskCreate((OS_TCB     *)&AppTaskPendTCB,                             //������ƿ��ַ
                 (CPU_CHAR   *)"App Task Pend",                             //��������
                 (OS_TASK_PTR ) AppTaskPend,                                //������
                 (void       *) 0,                                          //���ݸ����������β�p_arg����ʵ��
                 (OS_PRIO     ) APP_TASK_PEND_PRIO,                         //��������ȼ�
                 (CPU_STK    *)&AppTaskPendStk[0],                          //�����ջ�Ļ���ַ
                 (CPU_STK_SIZE) APP_TASK_PEND_STK_SIZE / 10,                //�����ջ�ռ�ʣ��1/10ʱ����������
                 (CPU_STK_SIZE) APP_TASK_PEND_STK_SIZE,                     //�����ջ�ռ䣨��λ��sizeof(CPU_STK)��
                 (OS_MSG_QTY  ) 5u,                                         //����ɽ��յ������Ϣ��
                 (OS_TICK     ) 0u,                                         //�����ʱ��Ƭ��������0��Ĭ��ֵOSCfg_TickRate_Hz/10��
                 (void       *) 0,                                          //������չ��0����չ��
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //����ѡ��
                 (OS_ERR     *)&err);                                       //���ش�������
    
		OSTaskDel ( & AppTaskStartTCB, & err );                     //ɾ����ʼ������������������
		
		
}


/*
*********************************************************************************************************
*                                          POST TASK
*********************************************************************************************************
*/
static  void  AppTaskPost ( void * p_arg )
{
	OS_ERR      err;

	
	(void)p_arg;

					 
	while (DEF_TRUE) {                                                     //������
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON ) //���KEY1������
		{		                                                    //����LED1
			printf("KEY1������\n");
			OSFlagPost ((OS_FLAG_GRP  *)&flag_grp,                             //����־���BIT0��1
                  (OS_FLAGS      )KEY1_EVENT,
                  (OS_OPT        )OS_OPT_POST_FLAG_SET,
                  (OS_ERR       *)&err);

		}

		if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON ) //���KEY2������
		{		                                                    //����LED2
			printf("KEY2������\n");
			OSFlagPost ((OS_FLAG_GRP  *)&flag_grp,                             //����־���BIT1��1
                  (OS_FLAGS      )KEY2_EVENT,
                  (OS_OPT        )OS_OPT_POST_FLAG_SET,
                  (OS_ERR       *)&err);

		}

		OSTimeDlyHMSM ( 0, 0, 0, 20, OS_OPT_TIME_DLY, & err );  //ÿ20msɨ��һ��
		
	}

}


/*
*********************************************************************************************************
*                                          PEND TASK
*********************************************************************************************************
*/
static  void  AppTaskPend ( void * p_arg )
{
	OS_ERR      err;
  OS_FLAGS      flags_rdy;
	
	(void)p_arg;

					 
	while (DEF_TRUE) {                                       //������
  //�ȴ���־��ĵ�BIT0��BIT1������1 
  flags_rdy =   OSFlagPend ((OS_FLAG_GRP *)&flag_grp,                 
                (OS_FLAGS     )( KEY1_EVENT | KEY2_EVENT ),
                (OS_TICK      )0,
                (OS_OPT       )OS_OPT_PEND_FLAG_SET_ALL |
		                           OS_OPT_PEND_BLOCKING |
                                OS_OPT_PEND_FLAG_CONSUME,
                (CPU_TS      *)0,
                (OS_ERR      *)&err);
    if((flags_rdy & (KEY1_EVENT|KEY2_EVENT)) == (KEY1_EVENT|KEY2_EVENT)) 
    {
      /* ���������ɲ�����ȷ */
      printf ( "KEY1��KEY2������\n");		
      LED1_TOGGLE;       //LED1	��ת
    }
                                  

	}
	
}


