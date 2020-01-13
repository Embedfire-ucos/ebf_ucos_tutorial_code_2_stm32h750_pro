/*
  ******************************************************************************
  * @file    bsp.c
  * @author  fire
  * @version V1.0
  * @date    2016-xx-xx
  * @brief   系统初始化相关
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 STM32H750 开发板   
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************************
*/


/*
*********************************************************************************************************
*                                             包含的文件
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
* 函数名 : BSP_Init
* 描述   : 所有的硬件设备都应该放在这个函数里边初始化
* 形参   : 无
* 返回值 : 无
*********************************************************************************************************
*/
void  BSP_Init (void)
{
    BSP_OSTickInit();                  //初始化 OS 时钟源
    LED_GPIO_Config();                 //初始化LED
    USARTx_Config();                   //初始化 USART1
    Key_GPIO_Config();                //初始化按键
  
    /* 外部中断初始化 */
    EXTI_Key_Config();
}
/*
*********************************************************************************************************                               
* 函数名 : BSP_SystemClkCfg
* 描述   : 系统时钟初始化
* 形参   : 无
* 返回值 : 无
*********************************************************************************************************
*/

void  BSP_SystemClkCfg (void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /*使能供电配置更新 */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

  /* 当器件的时钟频率低于最大系统频率时，电压调节可以优化功耗，
		 关于系统频率的电压调节值的更新可以参考产品数据手册。  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* 启用HSE振荡器并使用HSE作为源激活PLL */
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
  
	/* 选择PLL作为系统时钟源并配置总线时钟分频器 */
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
* 函数名 : BSP_ClkFreqGet
* 描述   : 这个函数用来检索系统时钟频率
* 形参   : clk_id    系统时钟标识符
*                    BSP_CLK_ID_SYSCLK     系统时钟频率。
*                    BSP_CLK_ID_HCLK       CPU时钟频率。
*                    BSP_CLK_ID_PCLK1      APB1总线时钟频率。
*                    BSP_CLK_ID_PCLK2      APB2总线时钟频率。
* 返回值 : 无
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
             clk_freq = 1u;                                     //没有有效时钟频率
             break;
    }

    return (clk_freq);
}
/*
*********************************************************************************************************                               
* 函数名 : BSP_OSTickInit
* 描述   : 初始化 OS 嘀嗒时钟中断
* 形参   : 无
* 返回值 : 无
*********************************************************************************************************
*/
void  BSP_OSTickInit (void)
{
    CPU_INT32U  cpu_clk_freq;


    cpu_clk_freq = BSP_ClkFreqGet(BSP_CLK_ID_SYSCLK);             //获取CPU时钟，时间戳是以该时钟计数

  
    OS_CPU_SysTickInitFreq(cpu_clk_freq);//初始化uC/OS 周期时钟源 (SysTick)
}

/*
*********************************************************************************************************                               
* 函数名 : HAL_InitTick
* 描述   : 覆盖STM32F7xx HAL 库中的HAL_InitTick函数，因为Micrium实时系统有自己的Systick 初始化，
*          必须在多任务启动后才初始化滴答时钟
* 形参   : TickPriority     滴答中断优先级
* 返回值 : 无
*********************************************************************************************************
*/
HAL_StatusTypeDef  HAL_InitTick (uint32_t  TickPriority)
{
    HAL_NVIC_SetPriorityGrouping(4);

    return (HAL_OK);
}

/*
*********************************************************************************************************                               
* 函数名 : HAL_GetTick
* 描述   : 覆盖STM32F7xx HAL HAL_GetTick函数，因为Micrium实时系统有自己的滴答计时器的值
* 形参   : 无
* 返回值 : 滴答计时器的值
* 注意   ：请确保滴答时钟任务比应用程序启动任务具有更高的优先级
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
