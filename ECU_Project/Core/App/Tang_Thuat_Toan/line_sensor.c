#include "line_sensor.h"
#include "app_bsp.h"

// Định nghĩa vật lý các biến mảng toàn cục được khai báo trong app_bsp.h
uint16_t adc_values[8] = {0};
uint8_t is_calibrated = 0;
uint8_t black_count = 0;
uint16_t cal_min[8] = {4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095};
uint16_t cal_max[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Hàm tự động quét lấy mẫu bề mặt sa bàn để hiệu chuẩn cảm biến
void Auto_Calibration(void) {
    // 1. Bật nguồn dàn LED hồng ngoại QTR-8A qua chân PB13
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    osDelay(20); // Chờ điện áp LED ổn định

    // 2. Kích hoạt bộ chuyển đổi ADC1 quét liên tục bằng DMA đẩy vào mảng adc_values
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_values, 8);

    // 3. Vòng lặp hiệu chuẩn chạy trong 3 giây
    // Trong thời gian này, người giữ xe sẽ vuốt dàn mắt qua lại giữa vạch đen và nền trắng
    uint32_t startTime = HAL_GetTick();
    while (HAL_GetTick() - startTime < 3000) {
        for (int i = 0; i < 8; i++) {
            if (adc_values[i] < cal_min[i]) cal_min[i] = adc_values[i];
            if (adc_values[i] > cal_max[i]) cal_max[i] = adc_values[i];
        }
        osDelay(10); // Chu kỳ lấy mẫu hiệu chuẩn 10ms
    }
}

// Hàm làm sạch/đợi dữ liệu ADC (Vì DMA chạy ngầm liên tục nên hàm này đóng vai trò bộ lọc ổn định)
void Refresh_ADC(void) {
    // Dữ liệu trong mảng adc_values tự động cập nhật liên tục nhờ phần cứng DMA.
    // Thêm một khoảng trễ cực ngắn để tránh hiện tượng đọc dính nhiễu chuyển mạch.
    asm("NOP");
}

// Hàm tính toán vị trí tâm xe dựa trên thuật toán trung bình có trọng số
int Read_Position(void) {
    uint32_t avg = 0;
    uint32_t sum = 0;
    uint8_t on_line = 0;
    black_count = 0;

    for (int i = 0; i < 8; i++) {
        long value = adc_values[i];

        // Chuẩn hóa dữ liệu về khoảng 0 (Trắng) đến 1000 (Đen) dựa trên cal_min/max
        if (cal_max[i] != cal_min[i]) {
            value = ((value - cal_min[i]) * 1000) / (cal_max[i] - cal_min[i]);
        }

        // Ràng buộc giới hạn an toàn
        if (value < 0) value = 0;
        if (value > 1000) value = 1000;

        // Đếm số mắt đang thực sự đè lên vạch đen (Ngưỡng điện áp chuẩn hóa > 500)
        if (value > 500) {
            black_count++;
            on_line = 1;
        }

        // Áp trọng số vị trí tăng dần từ 0 (Mắt ngoài cùng bên trái) đến 7000 (Mắt ngoài cùng bên phải)
        // Khoảng cách giữa các mắt đều nhau 1000 đơn vị, Tâm xe hoàn hảo sẽ là 3500
        avg += (uint32_t)(value * (i * 1000));
        sum += (uint32_t)value;
    }

    // TÌNH HUỐNG KHẨN CẤP: Nếu tất cả các mắt đều đọc được màu trắng (Xe mất dấu đường line hoàn toàn)
    if (on_line == 0) {
        return -1; // Trả về -1 để kích hoạt Khối xử lý ngã rẽ/quay compa tìm vạch trong app_core.c
    }

    return (int)(avg / sum);
}
