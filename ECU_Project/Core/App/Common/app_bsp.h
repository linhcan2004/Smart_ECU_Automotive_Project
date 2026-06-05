#ifndef APP_BSP_H_
#define APP_BSP_H_

#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Ngoại vi phần cứng (MCAL)
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart6;
extern IWDG_HandleTypeDef hiwdg;

// Cờ điều khiển hệ thống
extern volatile uint8_t run_flag;
extern volatile uint8_t inject_error;
extern uint8_t is_calibrated;
extern uint8_t mpu_status;
extern uint8_t black_count;
extern volatile uint8_t is_calibrating;
typedef enum {
    CAR_STATE_IDLE       = 0,  // Đứng yên chờ lệnh
    CAR_STATE_CALIBRATE  = 1,  // Đang tự động học sa bàn
    CAR_STATE_TURNING    = 2,  // Đang xoay compa ôm cua tại ngã rẽ
    CAR_STATE_RUNNING    = 3   // Đang tính toán PID bám line đường thẳng
} CarState_t;

extern volatile CarState_t car_state;

// Biến cảm biến QTR và PID
extern uint16_t adc_values[8];
extern int setpoint;
extern int error;
extern int last_error;
extern int P, I, PID_value;
extern float filtered_D;
extern float Kp, Ki, Kd;
extern int base_speed, max_speed;

// Biến Encoder & Định vị
extern int32_t enc_left_count;
extern int32_t enc_right_count;
extern int16_t gyro_z_offset;

// Bộ đệm UART và Giám sát sức khỏe
extern char uart_buf[150];
extern uint32_t last_uart_time;
extern volatile uint8_t task_health_mask;

//Mảng lưu thông số Calibration cho 8 cảm biến QTR
extern uint16_t cal_min[8];
extern uint16_t cal_max[8];

// Đồng bộ FreeRTOS Tĩnh
extern osMutexId_t uartMutexHandle;
extern osMessageQueueId_t controlQueueHandle;

#endif /* APP_BSP_H_ */
