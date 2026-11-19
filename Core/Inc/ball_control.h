/**
 * @file    ball_control.h
 * @brief   钢球位置闭环控制模块（STM32F407ZGT6 端）
 *
 * 系统链路:
 *   K230(俯视识别钢球) --USART2 115200--> STM32(PID) --TIM5_CH1 PWM--> 舵机(摆杆)
 *
 * 依赖外设: USART2(PA2/PA3) + TIM5_CH1(PA0,50Hz) + TIM6(1ms节拍)
 * 供电: 锂电池 -> 升降压模块 -> 5V(舵机,独立供电) / 3.3V(STM32/K230), 共地
 */
#ifndef __BALL_CONTROL_H
#define __BALL_CONTROL_H

#include "stm32f4xx_hal.h"
#include <math.h>

/* ===================== 串口协议（12 字节帧） ===================== */
/* 字节: [0]AA [1]BB [2]状态 [3]X_H [4]X_L [5]Y_H [6]Y_L [7]半径 [8]POS_H [9]POS_L [10]帧ID [11]CC */
#define FRAME_HEADER0   0xAA
#define FRAME_HEADER1   0xBB
#define FRAME_TAIL      0xCC
#define FRAME_LEN       12

/* ===================== 钢球数据（来自 K230） ===================== */
typedef struct {
    int16_t pos_mm;     /* 钢球位置(mm), 右正左负（K230 反馈，PID 唯一反馈源） */
    uint8_t valid;      /* 1=收到新帧，Control_Task 消费后清零 */
    uint8_t lost_cnt;   /* 丢失计数（连续未收到数据的 1ms tick 数） */
} BallData_t;
extern BallData_t ball_data;

/* ===================== PID 控制器 ===================== */
typedef struct {
    float Kp, Ki, Kd;
    float integral;
    float prev_error;
    float target;           /* 目标位置(cm) */
    float output_min;       /* 输出限幅 */
    float output_max;
} PID_t;
extern PID_t pid;

/* ===================== 状态机 ===================== */
typedef enum {
    STATE_TO_NEG5 = 0,  /* 阶段1: 目标 -5cm */
    STATE_TO_POS5,      /* 阶段2: 目标 +5cm */
    STATE_DONE          /* 完成,保持 +5cm  */
} ControlState_t;
extern ControlState_t ctrl_state;

/* ===================== 调参宏 ===================== */
#define ERROR_THRESHOLD   0.8f    /* cm, 误差<阈值才切换目标 */
#define LOST_LIMIT        100     /* 1ms tick 数, 超过认为 K230 掉线 */

/* ===================== 舵机参数（55°-110°硬限位,70°平衡中心） ===================== */
/* 标准舵机映射: 500us=0°, 2500us=180°, 14us/度 */
#define SERVO_MIN_PW      1100   /* us, 55° (硬性下限) */
#define SERVO_MAX_PW      1750   /* us, 110° (硬性上限) */
#define SERVO_CENTER      1350   /* us, 70° (钢球平衡中心) */

/* ===================== API ===================== */
void BallControl_Init(void);      /* 初始化: PWM + USART2 + PID 参数 */
int  BallControl_Poll(void);      /* 主循环轮询, 收到完整帧返回1 */
void BallControl_Task(void);      /* PID 控制, 收到新帧后调用 */
void BallControl_SetTarget(float cm);

#endif /* __BALL_CONTROL_H */
