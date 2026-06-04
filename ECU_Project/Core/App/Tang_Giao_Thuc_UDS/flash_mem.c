#include "flash_mem.h"
#include "app_bsp.h"
#include <string.h>

// Khai báo một chuỗi số VIN mặc định nếu xe chưa từng được định danh
static const char DEFAULT_VIN[] = "VNUUETHANOI2026X";

// Hàm tự động nạp dữ liệu từ Flash vào RAM khi xe vừa khởi động nổ máy
void Flash_Load_Config(void) {
    ECU_Storage_t temp_config;

    // Đọc trực tiếp byte từ địa chỉ Flash vật lý sao chép vào biến struct tạm
    memcpy(&temp_config, (void*)FLASH_USER_START_ADDR, sizeof(ECU_Storage_t));

    // Kiểm tra xem phân vùng này đã từng được ghi dữ liệu chuẩn chưa
    if (temp_config.magic_number == 0x55AA55AA) {
        // Nếu đã có dữ liệu cũ: Nạp thẳng các hệ số PID và vận tốc cũ vào hệ thống cho Linh chạy
        Kp = temp_config.kp;
        Ki = temp_config.ki;
        Kd = temp_config.kd;
        base_speed = temp_config.base_speed;
        max_speed = temp_config.max_speed;
    } else {
        // Nếu là chip mới tinh chưa ghi gì: Thiết lập giá trị chạy nền an toàn ban đầu
        Kp = 0.28f;
        Ki = 0.001f;
        Kd = 0.65f;
        base_speed = 400;
        max_speed = 800;

        // Ghi đè cấu hình xuất xưởng này vào Flash ngay để lần sau không bị rơi vào nhánh này nữa
        Flash_Save_Config();
    }
}

// Hàm xóa và "khắc" dữ liệu từ RAM xuống ô nhớ Flash của chip STM32
void Flash_Save_Config(void) {
    ECU_Storage_t config_to_save;

    // 1. Đóng gói dữ liệu hiện tại từ các biến toàn cục vào Struct
    config_to_save.magic_number = 0x55AA55AA;
    config_to_save.kp = Kp;
    config_to_save.ki = Ki;
    config_to_save.kd = Kd;
    config_to_save.base_speed = base_speed;
    config_to_save.max_speed = max_speed;
    strcpy(config_to_save.vin_number, DEFAULT_VIN);

    // 2. Mở khóa bộ nhớ Flash của vi điều khiển
    HAL_FLASH_Unlock();

    // 3. Tiến hành XÓA SẠCH (Erase) Sector 7.
    // Nguyên lý Flash bắt buộc phải xóa toàn bộ phân vùng về trạng thái 0xFF trước khi ghi dữ liệu mới.
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;

    EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3; // Chạy ở dải điện áp tiêu chuẩn 3.3V
    EraseInitStruct.Sector        = FLASH_USER_SECTOR;
    EraseInitStruct.NbSectors     = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) == HAL_OK) {
        // 4. Lập trình ghi từng từ đơn (Word - 4 bytes) của struct vào Flash
        uint32_t *data_ptr = (uint32_t*)&config_to_save;
        uint32_t target_addr = FLASH_USER_START_ADDR;
        int words_to_write = sizeof(ECU_Storage_t) / 4;

        // Nếu kích thước struct không chia hết cho 4, cộng thêm 1 vòng lặp bảo vệ
        if (sizeof(ECU_Storage_t) % 4 != 0) words_to_write++;

        for (int i = 0; i < words_to_write; i++) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, target_addr, data_ptr[i]) == HAL_OK) {
                target_addr += 4; // Tăng địa chỉ lên 4 bytes cho từ tiếp theo
            } else {
                // Xử lý lỗi ghi phần cứng nếu cần thiết
                break;
            }
        }
    }

    // 5. Khóa cứng bộ nhớ Flash lại để chống ghi nhầm khi xe chạy rung lắc
    HAL_FLASH_Lock();
}
