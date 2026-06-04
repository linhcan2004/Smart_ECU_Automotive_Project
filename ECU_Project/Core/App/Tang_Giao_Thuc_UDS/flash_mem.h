#ifndef FLASH_MEM_H_
#define FLASH_MEM_H_

#include "main.h"

// Chọn Sector 7 (Vùng nhớ cuối cùng của chip STM32F401RE - Địa chỉ 0x08060000)
// Việc dùng phân vùng cuối giúp đảm bảo không bao giờ bị ghi đè vào code chương trình chính
#define FLASH_USER_START_ADDR   0x08060000
#define FLASH_USER_SECTOR       FLASH_SECTOR_7

// Cấu trúc Struct đóng gói toàn bộ dữ liệu cấu hình cần lưu trữ an toàn của xe
typedef struct {
    uint32_t magic_number;  // Mã nhận diện (0x55AA55AA) để biết Flash đã từng được ghi chưa
    float kp;
    float ki;
    float kd;
    int base_speed;
    int max_speed;
    char vin_number[18];   // Chuỗi 17 ký tự số VIN định danh xe tiêu chuẩn Automotive
} ECU_Storage_t;

// Khai báo các hàm đọc ghi Flash
void Flash_Load_Config(void);
void Flash_Save_Config(void);

#endif
