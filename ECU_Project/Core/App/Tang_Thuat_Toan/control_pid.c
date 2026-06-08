#include "control_pid.h"
#include "app_bsp.h"

// Cấp phát vật lý các biến thuộc tầng Thuật toán
int setpoint = 3500;
int error = 0, last_error = 0;
int P = 0, I = 0, PID_value = 0;
float filtered_D = 0.0f;
float Kp = 0.1f, Ki = 0.0f, Kd = 1.0f;
int base_speed = 320, max_speed = 999;

void Reset_PID(void) {
    error      = 0;
    last_error = 0;
    I          = 0;
    filtered_D = 0.0f;
    PID_value  = 0;
}

void Motor_Drive(int left_speed, int right_speed) {
    if (left_speed  >  max_speed) left_speed  =  max_speed;
    if (left_speed  < -max_speed) left_speed  = -max_speed;
    if (right_speed >  max_speed) right_speed =  max_speed;
    if (right_speed < -max_speed) right_speed = -max_speed;

    if (left_speed >= 0) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, left_speed);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, -left_speed);
    }

    if (right_speed >= 0) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, right_speed);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, -right_speed);
    }
}
