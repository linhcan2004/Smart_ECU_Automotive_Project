#include "mpu6500.h"
#include "app_bsp.h"

uint8_t mpu_status = 0;
int16_t gyro_z_offset = 0;

int16_t Read_Gyro_Z(void) {
    uint8_t data[2];
    if (HAL_I2C_Mem_Read(&hi2c1, 0xD0, 0x47, 1, data, 2, 100) == HAL_OK) {
        return (int16_t)((data[0] << 8) | data[1]);
    }
    return 0;
}