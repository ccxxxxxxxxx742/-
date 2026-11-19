/**
 * @file    main.h
 * @brief   钢球位置闭环控制系统 - 主头文件
 *          STM32F407ZGT6 + K230 摄像头 + 舵机(摆杆)
 */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ====== 引脚定义 ====== */
/* USART2: PA2(TX) -> K230 RX,  PA3(RX) <- K230 TX */
#define K230_UART_TX_PIN    GPIO_PIN_2
#define K230_UART_TX_PORT   GPIOA
#define K230_UART_RX_PIN    GPIO_PIN_3
#define K230_UART_RX_PORT   GPIOA

/* 舵机 PWM: TIM5_CH1 = PA0 */
#define SERVO_PWM_PIN       GPIO_PIN_0
#define SERVO_PWM_PORT      GPIOA

/* ====== 外设句柄声明（定义在 main.c） ====== */
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef  htim5;
extern TIM_HandleTypeDef  htim6;

/* ====== 函数声明 ====== */
void Error_Handler(void);
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);
void MX_TIM5_Init(void);
void MX_TIM6_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
