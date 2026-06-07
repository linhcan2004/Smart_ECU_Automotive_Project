/**
 * @file    encoder.c
 * @brief   Module đọc xung Encoder bánh xe bằng TIM2 và TIM3
 */

#include "encoder.h"
#include "app_bsp.h"

volatile int32_t enc_left_count = 0;
volatile int32_t enc_right_count = 0;

void Encoder_Init(void)
{
    /* Kích hoạt chế độ Encoder Interface cho TIM2 và TIM3 */
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

    /* Đưa bộ đếm phần cứng về giá trị ban đầu */
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);

    /* Xóa dữ liệu đếm xung phía phần mềm */
    enc_left_count  = 0;
    enc_right_count = 0;
}


// Cập nhật số xung Encoder trong chu kỳ điều khiển hiện tại (20ms)
void Encoder_Update(void)
{
    enc_left_count  = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
    enc_right_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

    /* Xóa bộ đếm phần cứng ngay lập tức để chuẩn bị cho chu kỳ 20ms tiếp theo */
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);

    /* Xóa dữ liệu đếm xung phía phần mềm */
    enc_left_count  = 0;
    enc_right_count = 0;
}

/**
 * @brief Cập nhật số xung Encoder trong chu kỳ điều khiển hiện tại
 *
 * Đọc số xung thu được từ TIM2 và TIM3 trong khoảng thời gian 20ms,
 * sau đó đặt lại bộ đếm để chuẩn bị cho lần lấy mẫu tiếp theo.
 */
void Encoder_Update(void)
{
    /* Đọc giá trị bộ đếm Encoder */
    enc_left_count  = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
    enc_right_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

    /* Xóa bộ đếm để bắt đầu chu kỳ đo mới */
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_SET_COUNTER(&htim3, 0);
}