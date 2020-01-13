/*
  ******************************************************************************
  * @file    app_cfg.h
  * @author  fire
  * @version V1.0
  * @date    2016-xx-xx
  * @brief   uCOS-IIIӦ������ͷ�ļ�
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32H750 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************************
*/

#ifndef  APP_CFG_MODULE_PRESENT
#define  APP_CFG_MODULE_PRESENT

/*
*********************************************************************************************************
*                                           ģ�����/ʹ��
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            �������ȼ�
*********************************************************************************************************
*/

#define  APP_TASK_START_PRIO                2u
#define  APP_TASK_LED1_PRIO                 3u
#define  APP_TASK_LED2_PRIO                 3u
#define  APP_TASK_LED3_PRIO                 3u
/*
*********************************************************************************************************
*                                            �����ջ��С
*********************************************************************************************************
*/
#define  APP_TASK_START_STK_SIZE            256u
#define  APP_TASK_LED1_STK_SIZE             512u
#define  APP_TASK_LED2_STK_SIZE             512u
#define  APP_TASK_LED3_STK_SIZE             512u
/*
*********************************************************************************************************
*                                           ����/��������
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                        0u
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                       1u
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                        2u
#endif

#define  APP_TRACE_LEVEL                        TRACE_LEVEL_OFF
#define  APP_TRACE                              printf

#define  APP_TRACE_INFO(x)               ((APP_TRACE_LEVEL >= TRACE_LEVEL_INFO)  ? (void)(APP_TRACE x) : (void)0)
#define  APP_TRACE_DBG(x)                ((APP_TRACE_LEVEL >= TRACE_LEVEL_DBG)   ? (void)(APP_TRACE x) : (void)0)

#endif
