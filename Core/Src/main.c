/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 第 4-5 周作业：定时器与 PWM
  *
  *   任务一：TIM2 以"计数方式"定时 2 s，控制 LED 周期性亮-灭；
  *   任务二：TIM2_CH2 输出 PWM，让 LED 呈现呼吸灯效果（渐亮渐灭），
  *           一个完整呼吸周期约 1.4 s，可用 Keil 逻辑分析仪 / 示波器观察 PA1。
  *
  *   两种效果编译在同一个工程里，靠下面的 APP_TASK 宏切换。
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* ===================== 实验任务选择（★ 改这里） =====================
 *  改完这一行，重新编译下载，即可切换要做的任务：
 *      TASK_1_TIMER_COUNT : 定时器计数方式，LED 每 2 秒亮灭一次
 *      TASK_2_PWM_BREATH  : PWM 呼吸灯，完整呼吸周期约 1.4 秒
 * ==================================================================== */
#define TASK_1_TIMER_COUNT   0
#define TASK_2_PWM_BREATH    1

#define APP_TASK   TASK_1_TIMER_COUNT        /* ★★ 改这一行 ★★ */

/* ---- 定时器参数 ----
 * TIM2 挂接在 APB1 上。APB1 分频系数 = 2 时，定时器时钟 = PCLK1 x 2，
 * 所以系统时钟 72 MHz 时 TIM2CLK = 72 MHz。
 *   f_PWM = TIM2CLK / ((PSC + 1) * (ARR + 1))
 *         = 72 MHz / (72 * 1000)
 *         = 1 kHz
 * 1 kHz 远高于人眼分辨能力，所以 LED 看起来是"连续亮度"。
 */
#define TIM2_PRESCALER       71U            /* 72 MHz / 72 = 1 MHz 计数频率 */
#define TIM2_PERIOD          999U           /* 1 MHz / 1000 = 1 kHz PWM    */

/* 任务一：计数 2000 个 1 ms 中断 = 2 s，翻转一次 LED */
#define BLINK_COUNT_PERIOD   2000U

/* 任务二：呼吸灯用 100 级亮度，每级 7 ms，
 * 渐亮 700 ms + 渐暗 700 ms ≈ 1.4 s，符合题目 1~2 s 的要求。 */
#define BREATH_STEPS         100U
#define BREATH_STEP_MS       7U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
#if (APP_TASK == TASK_1_TIMER_COUNT)
static volatile uint16_t g_blink_cnt = 0U;   /* 已累计的定时器中断次数 */
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN PFP */
#if (APP_TASK == TASK_1_TIMER_COUNT)
static void LED_InitAsGpioOutput(void);
#endif
#if (APP_TASK == TASK_2_PWM_BREATH)
static void LED_PWM_Breath(void);
#endif
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#if (APP_TASK == TASK_1_TIMER_COUNT)
/**
  * @brief  把 PA1 配置成普通推挽输出（任务一用 GPIO 控制电平，
  *         任务二才把 PA1 复用成 TIM2_CH2 的复用推挽）。
  */
static void LED_InitAsGpioOutput(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin   = LED_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

  /* 本板 LED 为高电平点亮，先输出低电平（灭） */
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  定时器更新中断回调：每 1 ms 进入一次，累计 2000 次即 2 s
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    g_blink_cnt++;
    if (g_blink_cnt >= BLINK_COUNT_PERIOD)
    {
      g_blink_cnt = 0U;
      HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);   /* 亮 -> 灭 / 灭 -> 亮 */
    }
  }
}
#endif /* TASK_1_TIMER_COUNT */

#if (APP_TASK == TASK_2_PWM_BREATH)
/**
  * @brief  PWM 呼吸灯：让占空比按二次曲线从 0 渐变到 100%，再渐变回 0
  *         - 用二次曲线而不是直线，是因为人眼对亮度近似对数响应，
  *           二次曲线看起来"渐变更均匀"，不会前半段显得太亮太快。
  *         - CCR = (i * i) / (BREATH_STEPS * BREATH_STEPS) * (ARR + 1)
  */
static void LED_PWM_Breath(void)
{
  uint32_t i;
  uint16_t duty;

  /* ---------- 渐亮：占空比 0% -> 100% ---------- */
  for (i = 0U; i <= BREATH_STEPS; i++)
  {
    duty = (uint16_t)((i * i * (TIM2_PERIOD + 1U)) /
                      ((uint32_t)BREATH_STEPS * BREATH_STEPS));
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, duty);
    HAL_Delay(BREATH_STEP_MS);
  }

  /* ---------- 渐暗：占空比 100% -> 0% ---------- */
  for (i = BREATH_STEPS; i > 0U; i--)
  {
    duty = (uint16_t)((i * i * (TIM2_PERIOD + 1U)) /
                      ((uint32_t)BREATH_STEPS * BREATH_STEPS));
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, duty);
    HAL_Delay(BREATH_STEP_MS);
  }
}
#endif /* TASK_2_PWM_BREATH */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

#if (APP_TASK == TASK_1_TIMER_COUNT)
  /* ============ 任务一：定时器计数方式 ============
   * 1. PA1 先当普通 GPIO 用（LED 只需亮/灭两种状态）；
   * 2. 打开 TIM2 更新中断，计数器每 1 ms 溢出一次；
   * 3. 中断里数够 2000 次（= 2 s）才翻转一次 LED。
   */
  LED_InitAsGpioOutput();

  HAL_NVIC_SetPriority(TIM2_IRQn, 1U, 0U);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);

  __HAL_TIM_SET_COUNTER(&htim2, 0U);
  HAL_TIM_Base_Start_IT(&htim2);          /* 只启动计数，不启动 PWM 通道 */

#elif (APP_TASK == TASK_2_PWM_BREATH)
  /* ============ 任务二：PWM 呼吸灯 ============
   * 启动 TIM2_CH2 的 PWM 输出，PA1 上就会出现 1 kHz 方波，
   * 占空比由主循环里的 CCR2 决定。
   */
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

#if (APP_TASK == TASK_1_TIMER_COUNT)
    /* 任务一的所有工作都在定时器中断里完成，主循环空转即可 */
    __NOP();

#elif (APP_TASK == TASK_2_PWM_BREATH)
    /* 任务二：主循环不断刷新占空比，形成呼吸效果 */
    LED_PWM_Breath();

#endif

    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  /* HSE 8 MHz 晶振 -> PLL x9 -> 72 MHz（蓝色小板标配）。
     万一板上晶振不起振，自动退回 HSI（8 MHz / 2 x 9 = 36 MHz），
     避免卡死在 Error_Handler。 */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* HSE 起振失败 -> 退回 HSI */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      Error_Handler();
    }
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = TIM2_PRESCALER;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = TIM2_PERIOD;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

  HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
