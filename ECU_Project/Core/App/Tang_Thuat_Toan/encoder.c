#include "encoder.h"
#include "app_bsp.h"

// Định nghĩa vật lý biến lưu số lượng xung đếm được từ mô-tơ N20
int32_t enc_left_count = 0;
int32_t enc_right_count = 0;

void Encoder_Init(void) {
    // Kích hoạt chế độ đếm xung Encoder Mode trên cả hai bộ định thời phần cứng TIM2 và TIM3
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

    // Đặt giá trị đếm ban đầu về 0
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
}
