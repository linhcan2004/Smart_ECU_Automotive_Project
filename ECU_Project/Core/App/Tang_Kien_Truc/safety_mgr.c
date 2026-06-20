#include "safety_mgr.h"
#include "app_bsp.h"
#include "app_core.h"
#include "mpu6500.h"
#include "control_pid.h" // Chứa Motor_Drive
#include "usart.h"
#include "iwdg.h"
#include <stdio.h>
#include <string.h>

static uint8_t failsafe_count = 0;
static char safety_log_buf[64];

// Khởi tạo Safety Manager
void Safety_Init(void) {
    failsafe_count = 0;
    task_health_mask = 0;
}

// Hàm Kích hoạt trạng thái an toàn khẩn cấp
void Safety_EnterSafeState(const char* reason) {
    // 1. Cắt lực truyền động ngay lập tức
    Motor_Drive(0, 0);

    // 2. Khóa trạng thái xe về IDLE, ép người dùng phải khởi động lại
    car_state = CAR_STATE_IDLE;
    run_flag = 0;

    // 3. Reset bộ lọc
    failsafe_count = 0;

    // 4. Bắn log khẩn cấp (Dùng cờ riêng để không dẫm đạp UART Task 3)
    sprintf(safety_log_buf, "SAFE STATE TRIGGERED: %s\r\n", reason);
    HAL_UART_Transmit(&huart6, (uint8_t*)safety_log_buf, strlen(safety_log_buf), 10);
}

// Hàm giám sát va chạm qua IMU
void Safety_CheckIMU(void) {
    // Chỉ kích nổ bẫy va chạm khi xe đang chạy thẳng thực sự
    if (run_flag == 1 && car_state == CAR_STATE_RUNNING) {
        int16_t gyro_z = Read_Gyro_Z() - gyro_z_offset;

        if (gyro_z > GYRO_CRASH_THRESHOLD || gyro_z < -GYRO_CRASH_THRESHOLD) {
            failsafe_count++;
            if (failsafe_count >= CRASH_FILTER_SAMPLES) {
                // Đủ 3 mẫu liên tiếp -> Kích nổ Safe State
                char reason_buf[30];
                sprintf(reason_buf, "GYRO_LIMIT (%d)", gyro_z);
                Safety_EnterSafeState(reason_buf);
            }
        } else {
            failsafe_count = 0; // Xóa đếm nếu mẫu bình thường
        }
    } else {
        failsafe_count = 0;
    }
}

// Hàm kiểm tra sức khỏe đa nhiệm
void Safety_MonitorTasks(void) {
    // Kiểm tra xem mặt nạ đã tụ hội đủ 3 cờ (0x07) chưa
    if (task_health_mask == TASK_HEALTH_FULL_MASK) {
        // Nuôi chó -> Hệ thống khỏe mạnh
        HAL_IWDG_Refresh(&hiwdg);

        // Reset mặt nạ về 0 để bắt đầu chu kỳ giám sát mới
        task_health_mask = 0;
    } else {
        // Có Task bị kẹt không nộp báo cáo điểm danh
        // Không nuôi chó -> IWDG sẽ tự reset ECU trong vòng 500ms
    }
}
