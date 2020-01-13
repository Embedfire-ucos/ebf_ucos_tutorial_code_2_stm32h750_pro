/*
  ******************************************************************************
  * @file    bsp.c
  * @author  fire
  * @version V1.0
  * @date    2016-xx-xx
  * @brief   ϵͳ��ʼ�����
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32H750 ������   
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************************
*/


/*
*********************************************************************************************************
*                                             �������ļ�
*********************************************************************************************************
*/

#include  <app_cfg.h>
#include  "os.h"
#include  "bsp.h"
#include "./led/bsp_led.h" 
#include "usart/bsp_usart.h"
#include "./key/bsp_key.h" 
#include "./exti/bsp_exti.h" 
#include  "stm32h7xx_hal.h"

/*
*********************************************************************************************************
* ������ : BSP_Init
* ����   : ���е�Ӳ���豸��Ӧ�÷������������߳�ʼ��
* �β�   : ��
* ����ֵ : ��
*********************************************************************************************************
*/
void  BSP_Init (void)
{
    BSP_OSTickInit();                  //��ʼ�� OS ʱ��Դ
    LED_GPIO_Config();                 //��ʼ��LED
    USARTx_Config();                   //��ʼ�� USART1
    Key_GPIO_Config();                //��ʼ������
  
    /* �ⲿ�жϳ�ʼ�� */
    EXTI_Key_Config();
}
/*
*********************************************************************************************************                               
* ������ : BSP_SystemClkCfg
* ����   : ϵͳʱ�ӳ�ʼ��
* �β�   : ��
* ����ֵ : ��
*********************************************************************************************************
*/

void  BSP_SystemClkCfg (void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /*ʹ�ܹ������ø��� */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

  /* ��������ʱ��Ƶ�ʵ������ϵͳƵ��ʱ����ѹ���ڿ����Ż����ģ�
		 ����ϵͳƵ�ʵĵ�ѹ����ֵ�ĸ��¿��Բο���Ʒ�����ֲᡣ  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* ����HSE������ʹ��HSE��ΪԴ����PLL */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
 
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
	/* ѡ��PLL��Ϊϵͳʱ��Դ����������ʱ�ӷ�Ƶ�� */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK  | \
																 RCC_CLOCKTYPE_HCLK    | \
																 RCC_CLOCKTYPE_D1PCLK1 | \
																 RCC_CLOCKTYPE_PCLK1   | \
                                 RCC_CLOCKTYPE_PCLK2   | \
																 RCC_CLOCKTYPE_D3PCLK1);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
}


/*
*********************************************************************************************************                               
* ������ : BSP_ClkFreqGet
* ����   : ���������������ϵͳʱ��Ƶ��
* �β�   : clk_id    ϵͳʱ�ӱ�ʶ��
*                    BSP_CLK_ID_SYSCLK     ϵͳʱ��Ƶ�ʡ�
*                    BSP_CLK_ID_HCLK       CPUʱ��Ƶ�ʡ�
*                    BSP_CLK_ID_PCLK1      APB1����ʱ��Ƶ�ʡ�
*                    BSP_CLK_ID_PCLK2      APB2����ʱ��Ƶ�ʡ�
* ����ֵ : ��
*********************************************************************************************************
*/
CPU_INT32U  BSP_ClkFreqGet (BSP_CLK_ID  clk_id)
{
    CPU_INT32U  clk_freq;


    switch (clk_id) {
        case BSP_CLK_ID_SYSCLK:
             clk_freq = HAL_RCC_GetSysClockFreq();
             break;


        case BSP_CLK_ID_HCLK:
             clk_freq = HAL_RCC_GetHCLKFreq();
             break;


        case BSP_CLK_ID_PCLK1:
             clk_freq = HAL_RCC_GetPCLK1Freq();
             break;


        case BSP_CLK_ID_PCLK2:
             clk_freq = HAL_RCC_GetPCLK2Freq();
             break;


        default:
             clk_freq = 1u;                                     //û����Чʱ��Ƶ��
             break;
    }

    return (clk_freq);
}
/*
*********************************************************************************************************                               
* ������ : BSP_OSTickInit
* ����   : ��ʼ�� OS ���ʱ���ж�
* �β�   : ��
* ����ֵ : ��
*********************************************************************************************************
*/
void  BSP_OSTickInit (void)
{
    CPU_INT32U  cpu_clk_freq;


    cpu_clk_freq = BSP_ClkFreqGet(BSP_CLK_ID_SYSCLK);             //��ȡCPUʱ�ӣ�ʱ������Ը�ʱ�Ӽ���

  
    OS_CPU_SysTickInitFreq(cpu_clk_freq);//��ʼ��uC/OS ����ʱ��Դ (SysTick)
}

/*
*********************************************************************************************************                               
* ������ : HAL_InitTick
* ����   : ����STM32F7xx HAL ���е�HAL_InitTick��������ΪMicriumʵʱϵͳ���Լ���Systick ��ʼ����
*          �����ڶ�����������ų�ʼ���δ�ʱ��
* �β�   : TickPriority     �δ��ж����ȼ�
* ����ֵ : ��
*********************************************************************************************************
*/
HAL_StatusTypeDef  HAL_InitTick (uint32_t  TickPriority)
{
    HAL_NVIC_SetPriorityGrouping(4);

    return (HAL_OK);
}

/*
*********************************************************************************************************                               
* ������ : HAL_GetTick
* ����   : ����STM32F7xx HAL HAL_GetTick��������ΪMicriumʵʱϵͳ���Լ��ĵδ��ʱ����ֵ
* �β�   : ��
* ����ֵ : �δ��ʱ����ֵ
* ע��   ����ȷ���δ�ʱ�������Ӧ�ó�������������и��ߵ����ȼ�
*********************************************************************************************************
*/
uint32_t  HAL_GetTick(void)
{
    CPU_INT32U  os_tick_ctr;
#if (OS_VERSION >= 30000u)
    OS_ERR      os_err;


    os_tick_ctr = OSTimeGet(&os_err);
#else
    os_tick_ctr = OSTimeGet();
#endif

    return os_tick_ctr;
}
