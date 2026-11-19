/**
 * @file    main.c
 * @brief   钢球位置闭环控制系统 - 主程序
 *          STM32F407ZGT6 + K230 摄像头 + 舵机(摆杆)
 *
 * 链路: K230(识别钢球) --USART2--> STM32(PID) --TIM5_CH1 PWM--> 舵机(摆杆)
 * 策略: 视觉闭环 + 误差驱动状态切换 + 大Kd小PI
 *
 * 时钟: HSE 8MHz -> PLL -> SYSCLK 168MHz, APB1=42MHz, APB2=84MHz
 *       TIM5/TIM6 在 APB1, 定时器时钟 = 84MHz
 */
#include "main.h"
#include "ball_control.h"

/* ====== 外设句柄 ====== */
UART_HandleTypeDef huart2;
TIM_HandleTypeDef  htim5;
TIM_HandleTypeDef  htim6;

/* ============================================================
 * 主函数
 * ============================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    /* USART2 由 BallControl_Init 直接寄存器配置, 不走 HAL */

    BallControl_Init();

    while (1)
    {
        /* 轮询 USART2, 收到完整帧时运行 PID */
        if (BallControl_Poll()) {
            BallControl_Task();
        }
    }
}

/* ============================================================
 * 系统时钟配置: HSE 8MHz -> PLL -> 168MHz
 *   PLL_M=8, PLL_N=336, PLL_P=2, PLL_Q=7
 *   SYSCLK=168MHz, HCLK=168MHz
 *   APB1=42MHz(/4), APB2=84MHz(/2)
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 8;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
                                | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1
                                | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;   /* 42MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;   /* 84MHz */

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * GPIO 初始化
 *   - 使能 GPIOA 时钟(USART2/TIM5 都在 PA 口)
 *   - 舵机 PWM 与 UART 的 GPIO 在各自 MspInit 中配置
 * ============================================================ */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PC13: LED (推挽输出, 低电平点亮) */
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);   /* 亮 */
}

/* ============================================================
 * USART2 初始化: 115200-8-N1, TX(PA2)/RX(PA3)
 * ============================================================ */
void MX_USART2_UART_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * TIM5 初始化: CH1(PA0) PWM, 50Hz
 *   定时器时钟 84MHz, PSC=83 -> 1MHz, ARR=20000 -> 50Hz
 * ============================================================ */
void MX_TIM5_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim5.Instance               = TIM5;
    htim5.Init.Prescaler         = 83;          /* 1MHz */
    htim5.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim5.Init.Period            = 20000;       /* 50Hz */
    htim5.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK) {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&htim5) != HAL_OK) {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    /* PWM1 模式, 初始脉宽 = SERVO_CENTER */
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = SERVO_CENTER;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * TIM6 初始化: 1ms 周期中断(1kHz)
 *   定时器时钟 84MHz, PSC=83 -> 1MHz, ARR=999 -> 1kHz
 * ============================================================ */
void MX_TIM6_Init(void)
{
    htim6.Instance               = TIM6;
    htim6.Init.Prescaler         = 83;          /* 1MHz */
    htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim6.Init.Period            = 999;         /* 1kHz = 1ms */
    htim6.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * 错误处理
 * ============================================================ */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        for (volatile uint32_t d = 0; d < 5000000; d++);  /* 简单延时 */
    }
}

/* USE_FULL_ASSERT 断言失败回调 */
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
    while (1) { }
}
#endif
