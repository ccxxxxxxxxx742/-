/**
 * @file    ball_control.c
 * @brief   钢球位置闭环控制模块实现 (轮询模式, 不依赖 HAL UART 中断)
 *
 * 核心思想: K230 实时识别钢球位置 -> USART2 -> STM32 轮询接收 -> PID -> 舵机
 */
#include "ball_control.h"
#include "main.h"

/* ===================== 全局实例 ===================== */
BallData_t      ball_data  = {0};
PID_t           pid        = {0};
ControlState_t  ctrl_state = STATE_TO_NEG5;

/* ===================== UART 接收状态机 ===================== */
typedef enum {
    RX_WAIT_AA = 0,
    RX_WAIT_BB,
    RX_PAYLOAD
} RxState_t;

static RxState_t rx_state   = RX_WAIT_AA;
static uint8_t   rx_buf[FRAME_LEN];
static uint8_t   rx_idx     = 0;

/* ===================== 内部函数 ===================== */
static void  Frame_Parse(const uint8_t *frame);
static float PID_Calculate(PID_t *p, float measurement);
static void  Servo_SetPW(uint32_t pulse_us);
static uint32_t PID_To_Servo(float pid_out);

/* =========================================================
 * 初始化
 * ========================================================= */
void BallControl_Init(void)
{
    /* === TIM5 CH1 PWM: 直接寄存器配置 (50Hz, 舵机) === */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_0;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    TIM5->PSC  = 83;
    TIM5->ARR  = 20000;
    TIM5->CCR1 = SERVO_CENTER;
    TIM5->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
    TIM5->CCER  = TIM_CCER_CC1E;
    TIM5->CR1   = TIM_CR1_CEN;

    /* === USART2 直接寄存器配置 (绕过 HAL) === */
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2: 115200-8-N1, APB1=42MHz */
    USART2->BRR = 42000000U / 115200U;   /* = 365 */
    USART2->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;

    /* === PID 参数 === */
    pid.Kp         = 20.0f;
    pid.Ki         = 0.020f;       /* 开积分: 消除稳态误差, 慢慢推到中心 */
    pid.Kd         = 150.0f;
    pid.integral   = 0.0f;
    pid.prev_error = 0.0f;
    pid.target     = 0.0f;       /* 目标: 球在中心 */
    pid.output_min = -210.0f;     /* 70°->55° */
    pid.output_max =  560.0f;     /* 70°->110° */

    ctrl_state       = STATE_TO_NEG5;
    ball_data.pos_mm = 0;
    ball_data.valid  = 0;
    rx_state = RX_WAIT_AA;
    rx_idx   = 0;
}

void BallControl_SetTarget(float cm)
{
    pid.target     = cm;
    pid.integral   = 0.0f;
    pid.prev_error = 0.0f;
}

/* =========================================================
 * 轮询接收: 在主循环中调用, 处理收到的每个字节
 * 收到完整帧时返回 1, 否则返回 0
 * ========================================================= */
int BallControl_Poll(void)
{
    uint32_t sr = USART2->SR;

    /* 清除 ORE (溢出错误): 读 SR 再读 DR */
    if (sr & USART_SR_ORE) {
        (void)USART2->DR;
        return 0;
    }

    if (!(sr & USART_SR_RXNE)) {
        return 0;
    }

    uint8_t byte = (uint8_t)(USART2->DR & 0xFF);

    /* 每收到一个字节翻转 LED (证明 UART 在收数据) */
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    switch (rx_state) {
    case RX_WAIT_AA:
        if (byte == FRAME_HEADER0) {
            rx_buf[0] = byte;
            rx_state  = RX_WAIT_BB;
        }
        break;

    case RX_WAIT_BB:
        if (byte == FRAME_HEADER1) {
            rx_buf[1] = byte;
            rx_idx    = 2;
            rx_state  = RX_PAYLOAD;
        } else if (byte == FRAME_HEADER0) {
            /* 连续 AA, 保持等待 BB */
        } else {
            rx_state = RX_WAIT_AA;
        }
        break;

    case RX_PAYLOAD:
        rx_buf[rx_idx++] = byte;
        if (rx_idx >= FRAME_LEN) {
            rx_state = RX_WAIT_AA;
            if (rx_buf[FRAME_LEN - 1] == FRAME_TAIL) {
                Frame_Parse(rx_buf);
                return 1;
            }
        }
        break;

    default:
        rx_state = RX_WAIT_AA;
        break;
    }

    return 0;
}

/* 解析一帧有效数据 */
static void Frame_Parse(const uint8_t *frame)
{
    /* 收到完整帧, 翻转 LED */
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    /* frame[2] = status: 0x00=无球, 0x01=有球 */
    if (frame[2] == 0x00) {
        ball_data.valid = 0;
        return;
    }

    /* POS = 钢球位置(mm, 有符号 int16, 右正左负) */
    int16_t pos = (int16_t)(((uint16_t)frame[8] << 8) | frame[9]);

    ball_data.pos_mm   = pos;
    ball_data.valid    = 1;
    ball_data.lost_cnt = 0;
}

/* =========================================================
 * PID 计算
 * ========================================================= */
static float PID_Calculate(PID_t *p, float measurement)
{
    float error = measurement - p->target;   /* 取反: 负反馈 */

    p->integral += error;
    if (p->integral >  200.0f) p->integral =  200.0f;
    if (p->integral < -200.0f) p->integral = -200.0f;

    float derivative = error - p->prev_error;
    p->prev_error = error;

    float output = p->Kp * error
                 + p->Ki * p->integral
                 + p->Kd * derivative;

    if (output > p->output_max) output = p->output_max;
    if (output < p->output_min) output = p->output_min;

    return output;
}

/* =========================================================
 * 舵机驱动
 * ========================================================= */
static void Servo_SetPW(uint32_t pulse_us)
{
    if (pulse_us < SERVO_MIN_PW) pulse_us = SERVO_MIN_PW;
    if (pulse_us > SERVO_MAX_PW) pulse_us = SERVO_MAX_PW;
    TIM5->CCR1 = pulse_us;
}

static uint32_t PID_To_Servo(float pid_out)
{
    float pw = (float)SERVO_CENTER + pid_out;
    return (uint32_t)pw;
}

/* =========================================================
 * 控制任务: 每收到一帧数据调用一次
 * ========================================================= */
void BallControl_Task(void)
{
    float current_pos = (float)ball_data.pos_mm / 10.0f;   /* mm -> cm */
    float output = PID_Calculate(&pid, current_pos);
    Servo_SetPW(PID_To_Servo(output));
}
