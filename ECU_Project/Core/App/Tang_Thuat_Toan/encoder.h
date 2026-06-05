#ifndef ENCODER_H_
#define ENCODER_H_

#include "main.h"

/**
 * @brief Khởi tạo chế độ đếm xung phần cứng (Encoder Interface Mode)
 * @note  Cấu hình cho các bộ định thời phần cứng (TIM2 và TIM3)
 */
void Encoder_Init(void);

/**
 * @brief Cập nhật dữ liệu bộ đếm xung phục vụ tính toán động học phương tiện
 * @note  Được gọi định kỳ trong tác vụ điều khiển vòng kín (Control Task)
 */
void Encoder_Update(void);

#endif