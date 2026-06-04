#include "control_pid.h"
#include "app_bsp.h"
#include <stdlib.h>

// Định nghĩa vật lý toàn bộ các biến thuật toán điều khiển kín từ app_bsp.h
int setpoint = 3500; // Vị trí mong muốn (Chính giữa vạch làn đường)
int error = 0;
int last_error = 0;
int P = 0, I = 0, PID_value = 0;
float filtered_D = 0.0f;

// Hệ số cấu hình PID bám làn mặc định (Cậu có thể tinh chỉnh các số này khi chạy thử sa bàn)
float Kp = 0.28f;
float Ki = 0.001f;
float Kd = 0.65f;

int base_speed = 400; // Tốc độ chạy nền cơ bản của xe (Dải điều khiển từ 0 - 1000)
int max_speed = 800;  // Giới hạn tốc độ tối đa bảo vệ phần cứng động cơ

// Hàm dọn sạch các biến tích phân và vi phân khi xe cần chuyển trạng thái hoặc bắt lại làn mới
void Reset_PID(void) {
    error = 0;
    last_error = 0;
    P = 0;
    I = 0;
    filtered_D = 0.0f;
    PID_value = 0;
}

// Hàm trực tiếp xuất tín hiệu điều khiển xuống chip cầu H Driver TB6612
void Motor_Drive(int left_speed, int right_speed) {

    // 1. Kích hoạt chân Standby (PA10) lên mức HIGH để mở khóa cho phép Driver hoạt động
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);

    // 2. XỬ LÝ ĐIỀU KHIỂN BÁNH TRÁI (PWM: TIM1_CH1, Hướng: PB10/AIN1, PB4/AIN2)
    if (left_speed >= 0) {
        // Bánh trái quay tiến
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);   // AIN1 = 1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_RESET); // AIN2 = 0
    } else {
        // Bánh trái quay lùi
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET); // AIN1 = 0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4,  GPIO_PIN_SET);   // AIN2 = 1
        left_speed = -left_speed; // Lấy giá trị tuyệt đối để băm xung PWM
    }

    // 3. XỬ LÝ ĐIỀU KHIỂN BÁNH PHẢI (PWM: TIM1_CH2, Hướng: PB5/BIN1, PB3/BIN2)
    if (right_speed >= 0) {
        // Bánh phải quay tiến
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_SET);   // BIN1 = 1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET); // BIN2 = 0
    } else {
        // Bánh phải quay lùi
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,  GPIO_PIN_RESET); // BIN1 = 0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_SET);   // BIN2 = 1
        right_speed = -right_speed; // Lấy giá trị tuyệt đối để băm xung PWM
    }

    // 4. Giới hạn xung PWM tối đa để không vượt quá chu kỳ tràn (ARR) của Timer 1 (Thường cấu hình là 1000)
    if (left_speed > 1000)  left_speed = 1000;
    if (right_speed > 1000) right_speed = 1000;

    // 5. Nạp trực tiếp giá trị xung vào thanh ghi so sánh để thay đổi điện áp xuất ra động cơ
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, left_speed);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, right_speed);
}
