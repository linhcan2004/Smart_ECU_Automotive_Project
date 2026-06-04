#include "mpu6500.h"
#include "app_bsp.h"

uint8_t mpu_status = 0;
int16_t gyro_z_offset = 0;

void MPU6500_Init(void) {
    int32_t sg = 0;

    // Kiểm tra xem thiết bị I2C đã sẵn sàng chưa (Địa chỉ 0xD0)
    if (HAL_I2C_IsDeviceReady(&hi2c1, 0xD0, 3, 100) == HAL_OK) {
        mpu_status = 1;

        // Thanh ghi 0x6B: Ghi 0x00 để đánh thức MPU6500 thoát khỏi Sleep Mode
        uint8_t d = 0x00; 
        HAL_I2C_Mem_Write(&hi2c1, 0xD0, 0x6B, 1, &d, 1, 100);

        // Thanh ghi 0x1A: Ghi 0x06 để cấu hình bộ lọc thông thấp chống nhiễu gắt
        d = 0x06;          
        HAL_I2C_Mem_Write(&hi2c1, 0xD0, 0x1A, 1, &d, 1, 100);

        // Vòng lặp lấy mẫu 100 lần trước khi chạy để tính toán bù sai số tĩnh
        for (int i = 0; i < 100; i++) { 
            sg += Read_Gyro_Z(); 
            HAL_Delay(5); // Dùng HAL_Delay vì lúc này OS chưa khởi động
        }
        gyro_z_offset = (int16_t)(sg / 100);
    }
}

int16_t Read_Gyro_Z(void) {
    uint8_t data[2];
    if (HAL_I2C_Mem_Read(&hi2c1, 0xD0, 0x47, 1, data, 2, 100) == HAL_OK) {
        return (int16_t)((data[0] << 8) | data[1]);
    }
    return 0;
}
