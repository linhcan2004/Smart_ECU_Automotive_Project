#ifndef ENCODER_H_
#define ENCODER_H_

#include "main.h"

extern volatile int32_t enc_left_count;
extern volatile int32_t enc_right_count;

// Khởi tạo chế độ đếm xung phần cứng (Encoder Interface Mode)
// Cấu hình cho các bộ định thời phần cứng (TIM2 và TIM3)
void Encoder_Init(void);


// Cập nhật dữ liệu bộ đếm xung phục vụ tính toán động học phương tiện
// Được gọi định kỳ trong tác vụ điều khiển vòng kín (Control Task)
void Encoder_Update(void);

#endif
