#include "line_sensor.h"
#include "app_bsp.h"
#include "control_pid.h"
#include <stdio.h>
#include <string.h>

// Cấp phát vật lý các biến thuộc tầng cảm biến line
uint16_t adc_values[8] = {0};
uint8_t is_calibrated = 0;
uint8_t black_count = 0;
uint16_t cal_min[8] = {4095,4095,4095,4095,4095,4095,4095,4095};
uint16_t cal_max[8] = {0,0,0,0,0,0,0,0};

void Refresh_ADC(void) {
    if (__HAL_ADC_GET_FLAG(&hadc1, ADC_FLAG_OVR)) {
        __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_OVR);
    }
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_values, 8);
    osDelay(2); // Dùng osDelay của RTOS để nhường CPU trong 2ms chờ ADC quét xong
}

void Auto_Calibration(void) {
    HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_SET);
    uint32_t start_time = HAL_GetTick();
    uint8_t  phase = 0;         
    uint32_t phase_time = start_time;

    while (HAL_GetTick() - start_time < 5000) {
        HAL_IWDG_Refresh(&hiwdg); // Nuôi chó liên tục trong vòng lặp 5s tránh bị sập Reset
        Refresh_ADC();

        for (int i = 0; i < 8; i++) {
            if (adc_values[i] < cal_min[i]) cal_min[i] = adc_values[i];
            if (adc_values[i] > cal_max[i]) cal_max[i] = adc_values[i];
        }

        if (HAL_GetTick() - phase_time >= 500) {
            phase = !phase;
            phase_time = HAL_GetTick();
        }

        if (phase == 0) Motor_Drive(+180, -180);  
        else            Motor_Drive(-180, +180);  
    }

    Motor_Drive(0, 0); 
    HAL_GPIO_WritePin(GPIOA, LD2_Pin, GPIO_PIN_RESET);

    sprintf(uart_buf, "CALIBRATION DONE.\r\n");
    HAL_UART_Transmit(&huart6, (uint8_t*)uart_buf, strlen(uart_buf), 100);
}

int Read_Position(void) {
    uint32_t sum_weighted = 0;
    uint32_t sum = 0;
    black_count = 0; 

    for (int i = 0; i < 8; i++) {
        if (adc_values[i] > 3500) {
            sum          += (uint32_t)adc_values[i];
            sum_weighted += (uint32_t)adc_values[i] * (uint32_t)(i * 1000);
            black_count++; 
        }
    }
    if (sum == 0) return -1; 
    return (int)(sum_weighted / sum);
}