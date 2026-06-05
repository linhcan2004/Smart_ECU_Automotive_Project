#include "mpu6500.h"
#include "app_bsp.h"

uint8_t mpu_status = 0;
int16_t gyro_z_offset = 0;

void MPU6500_Init(void) {
    // 1. Kiểm tra xem thiết bị MPU6500 có thực sự kết nối vật lý và phản hồi không
    if (HAL_I2C_IsDeviceReady(&hi2c1, 0xD0, 3, 50) == HAL_OK) {
        mpu_status = 1; // Đánh dấu cảm biến hoạt động khỏe mạnh

        // Thanh ghi 0x6B: Đánh thức MPU6500 thoát khỏi Sleep Mode
        uint8_t d = 0x00; 
        HAL_I2C_Mem_Write(&hi2c1, 0xD0, 0x6B, 1, &d, 1, 50);

        // Thanh ghi 0x1A: Cấu hình bộ lọc thông thấp (DLPF) chống nhiễu gắt
        d = 0x06;          
        HAL_I2C_Mem_Write(&hi2c1, 0xD0, 0x1A, 1, &d, 1, 50);

        // Vòng lặp lấy mẫu bù sai số tĩnh (Offset)
        int32_t sg = 0;
        for (int i = 0; i < 100; i++) { 
            sg += Read_Gyro_Z(); 
            HAL_Delay(5);
        }
        gyro_z_offset = (int16_t)(sg / 100);
    }
    else {
        // --- BẪY LỖI AN TOÀN CHỦ ĐỘNG ---
        mpu_status = 0;    // Đánh dấu cảm biến đang bị lỗi/rơi dây
        gyro_z_offset = 0; // Đặt offset bằng 0 để tránh tính toán sai lệch
        // Hệ thống không bị treo, chip tiếp tục thoát ra ngoài để khởi động FreeRTOS!
    }
}

int16_t Read_Gyro_Z(void) {
    uint8_t data[2];
    if (HAL_I2C_Mem_Read(&hi2c1, 0xD0, 0x47, 1, data, 2, 100) == HAL_OK) {
        return (int16_t)((data[0] << 8) | data[1]);
    }
    return 0;
}
