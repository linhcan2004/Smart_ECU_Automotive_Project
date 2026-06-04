#include "uds_sm.h"
#include "app_bsp.h"
#include "flash_mem.h"
#include <string.h>

// Định nghĩa trạng thái bảo mật và phiên làm việc của UDS
typedef enum { UDS_SESSION_DEFAULT = 1, UDS_SESSION_EXTENDED = 3 } UDS_Session_t;
typedef enum { UDS_SEC_LOCKED = 0, UDS_SEC_SEED_SENT = 1, UDS_SEC_UNLOCKED = 2 } UDS_Security_t;

static UDS_Session_t current_session = UDS_SESSION_DEFAULT;
static UDS_Security_t security_status = UDS_SEC_LOCKED;
static uint32_t generated_seed = 0;

// Bộ đệm nhận dữ liệu UDS qua UART ngắt
uint8_t uds_rx_byte = 0;
uint8_t uds_rx_buffer[64] = {0};
volatile uint8_t uds_rx_index = 0;
volatile uint8_t uds_packet_ready = 0;

// Cấu trúc gói tin UDS qua UART quy ước: [Độ dài gói] [SID] [Data...]
void UDS_Init(void) {
    current_session = UDS_SESSION_DEFAULT;
    security_status = UDS_SEC_LOCKED;
    uds_rx_index = 0;
    uds_packet_ready = 0;
    // Kích hoạt ngắt nhận từng byte một từ cổng Bluetooth (USART6)
    HAL_UART_Receive_IT(&huart6, &uds_rx_byte, 1);
}

// Hàm Callback của thư viện HAL tự động gọi khi nhận đủ 1 byte qua ngắt UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART6) {
        if (uds_packet_ready == 0) {
            uds_rx_buffer[uds_rx_index] = uds_rx_byte;
            uds_rx_index++;

            // byte đầu tiên lưu độ dài toàn bộ gói tin
            if (uds_rx_index >= uds_rx_buffer[0] && uds_rx_buffer[0] > 0) {
                uds_packet_ready = 1; // Đánh dấu đã nhận đủ nguyên gói UDS
            }
        }
        // Tiếp tục bật cấu hình chờ byte tiếp theo
        HAL_UART_Receive_IT(&huart6, &uds_rx_byte, 1);
    }
}

// Hàm gửi phản hồi tiêu cực (Báo lỗi cho Máy tính Tester Tool)
static void UDS_SendNegativeResponse(uint8_t rejected_sid, uint8_t nrc) {
    uint8_t tx_buf[4];
    tx_buf[0] = 4;        // Độ dài gói tin (4 bytes)
    tx_buf[1] = 0x7F;     // 0x7F là định danh bắt buộc của Negative Response
    tx_buf[2] = rejected_sid;
    tx_buf[3] = nrc;
    HAL_UART_Transmit(&huart6, tx_buf, 4, 50);
}

// Hàm xử lý chính lõi Máy trạng thái dịch vụ UDS ISO 14229
void UDS_Process(void) {
    if (!uds_packet_ready) return;

    uint8_t len = uds_rx_buffer[0];
    uint8_t sid = uds_rx_buffer[1];
    uint8_t response_buffer[32] = {0};

    switch (sid) {
        // ---------------------------------------------------------------------
        // DỊCH VỤ 0x10: DIAGNOSTIC SESSION CONTROL (Quản lý phiên)
        // ---------------------------------------------------------------------
        case UDS_SID_DIAG_SESSION_CONTROL:
            if (len < 3) { UDS_SendNegativeResponse(sid, UDS_NRC_INCORRECT_MESSAGE_LENGTH); break; }
            uint8_t sub_session = uds_rx_buffer[2];

            if (sub_session == 0x01) {
                current_session = UDS_SESSION_DEFAULT;
                security_status = UDS_SEC_LOCKED; // Về phiên mặc định tự động khóa xe
            } else if (sub_session == 0x03) {
                current_session = UDS_SESSION_EXTENDED; // Mở rộng phiên cấu hình nâng cao
            } else {
                UDS_SendNegativeResponse(sid, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
                break;
            }
            response_buffer[0] = 3; response_buffer[1] = sid + 0x40; response_buffer[2] = sub_session;
            HAL_UART_Transmit(&huart6, response_buffer, 3, 50);
            break;

        // ---------------------------------------------------------------------
        // DỊCH VỤ 0x27: SECURITY ACCESS (Bảo mật Seed & Key)
        // ---------------------------------------------------------------------
        case UDS_SID_SECURITY_ACCESS:
            if (current_session != UDS_SESSION_EXTENDED) {
                UDS_SendNegativeResponse(sid, UDS_NRC_CONDITIONS_NOT_CORRECT);
                break;
            }
            uint8_t sec_sub = uds_rx_buffer[2];

            if (sec_sub == 0x01) { // BƯỚC 1: Tester xin mã Seed ẩn
                generated_seed = HAL_GetTick() + 0x1234; // Tạo Seed ngẫu nhiên theo thời gian chạy chip
                response_buffer[0] = 6; response_buffer[1] = sid + 0x40; response_buffer[2] = 0x01;
                memcpy(&response_buffer[3], &generated_seed, 4);
                security_status = UDS_SEC_SEED_SENT;
                HAL_UART_Transmit(&huart6, response_buffer, 7, 50);
            }
            else if (sec_sub == 0x02) { // BƯỚC 2: Tester gửi Key giải mã lên xe
                if (security_status != UDS_SEC_SEED_SENT) { UDS_SendNegativeResponse(sid, UDS_NRC_REQUEST_SEQUENCE_ERROR); break; }
                uint32_t client_key;
                memcpy(&client_key, &uds_rx_buffer[3], 4);

                // Thuật toán kiểm tra mã Key đối xứng: Key = Seed XOR thuật toán mã hóa 0x55AA55AA
                uint32_t expected_key = generated_seed ^ 0x55AA55AA;
                if (client_key == expected_key) {
                    security_status = UDS_SEC_UNLOCKED; // XE CHÍNH THỨC MỞ KHÓA BẢO MẬT KHÁNH THÀNH!
                    response_buffer[0] = 3; response_buffer[1] = sid + 0x40; response_buffer[2] = 0x02;
                    HAL_UART_Transmit(&huart6, response_buffer, 3, 50);
                } else {
                    security_status = UDS_SEC_LOCKED;
                    UDS_SendNegativeResponse(sid, UDS_NRC_INVALID_KEY);
                }
            } else {
                UDS_SendNegativeResponse(sid, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
            }
            break;

        // ---------------------------------------------------------------------
        // DỊCH VỤ 0x22: READ DATA BY IDENTIFIER (Đọc tham số xe)
        // ---------------------------------------------------------------------
        case UDS_SID_READ_DATA_BY_ID:
            if (len < 4) { UDS_SendNegativeResponse(sid, UDS_NRC_INCORRECT_MESSAGE_LENGTH); break; }
            uint16_t read_did = (uds_rx_buffer[2] << 8) | uds_rx_buffer[3];

            response_buffer[1] = sid + 0x40;
            response_buffer[2] = uds_rx_buffer[2];
            response_buffer[3] = uds_rx_buffer[3];

            if (read_did == UDS_DID_RUN_FLAG) {
                response_buffer[0] = 5; response_buffer[4] = run_flag;
                HAL_UART_Transmit(&huart6, response_buffer, 5, 50);
            }
            else if (read_did == UDS_DID_PID_GAINS) {
                response_buffer[0] = 16;
                memcpy(&response_buffer[4], &Kp, 4);
                memcpy(&response_buffer[8], &Ki, 4);
                memcpy(&response_buffer[12], &Kd, 4);
                HAL_UART_Transmit(&huart6, response_buffer, 16, 50);
            }
            else if (read_did == UDS_DID_ENCODER_DATA) {
                response_buffer[0] = 12;
                memcpy(&response_buffer[4], &enc_left_count, 4);
                memcpy(&response_buffer[8], &enc_right_count, 4);
                HAL_UART_Transmit(&huart6, response_buffer, 12, 50);
            }
            else if (read_did == UDS_DID_LINE_ERROR) {
                int16_t temp_err = (int16_t)error;
                response_buffer[0] = 6;
                response_buffer[4] = (uint8_t)((temp_err >> 8) & 0xFF);
                response_buffer[5] = (uint8_t)(temp_err & 0xFF);
                HAL_UART_Transmit(&huart6, response_buffer, 6, 50);
            }
            else {
                UDS_SendNegativeResponse(sid, 0x31); // Request Out Of Range
            }
            break;

        // ---------------------------------------------------------------------
        // DỊCH VỤ 0x2E: WRITE DATA BY IDENTIFIER (Ghi nạp tham số tinh chỉnh xe)
        // ---------------------------------------------------------------------
        case UDS_SID_WRITE_DATA_BY_ID:
            // Tính năng ghi cấu hình bắt buộc xe phải được Unlock Bảo mật (Dịch vụ 0x27) trước đó
            if (security_status != UDS_SEC_UNLOCKED) { UDS_SendNegativeResponse(sid, UDS_NRC_SECURITY_ACCESS_DENIED); break; }
            uint16_t write_did = (uds_rx_buffer[2] << 8) | uds_rx_buffer[3];

            if (write_did == UDS_DID_RUN_FLAG) {
                run_flag = uds_rx_buffer[4];
                response_buffer[0] = 4; response_buffer[1] = sid + 0x40; response_buffer[2] = uds_rx_buffer[2]; response_buffer[3] = uds_rx_buffer[3];
                HAL_UART_Transmit(&huart6, response_buffer, 4, 50);
            }
            else if (write_did == UDS_DID_PID_GAINS) {
                if (len < 16) { UDS_SendNegativeResponse(sid, UDS_NRC_INCORRECT_MESSAGE_LENGTH); break; }
                // Đọc trực tiếp byte thô từ PC ép kiểu nạp thẳng vào biến float thuật toán PID của Linh
                memcpy(&Kp, &uds_rx_buffer[4], 4);
                memcpy(&Ki, &uds_rx_buffer[8], 4);
                memcpy(&Kd, &uds_rx_buffer[12], 4);

                Flash_Save_Config();

                response_buffer[0] = 4; response_buffer[1] = sid + 0x40; response_buffer[2] = uds_rx_buffer[2]; response_buffer[3] = uds_rx_buffer[3];
                HAL_UART_Transmit(&huart6, response_buffer, 4, 50);
            }
            else {
                UDS_SendNegativeResponse(sid, 0x31);
            }
            break;

        // ---------------------------------------------------------------------
        // DỊCH VỤ 0x11: ECU RESET (Kích hoạt lệnh Khởi động lại hệ thống)
        // ---------------------------------------------------------------------
        case UDS_SID_ECU_RESET:
            if (uds_rx_buffer[2] == 0x01) { // Lệnh Hard Reset phần cứng
                response_buffer[0] = 3; response_buffer[1] = sid + 0x40; response_buffer[2] = 0x01;
                HAL_UART_Transmit(&huart6, response_buffer, 3, 50);
                osDelay(50); // Chờ truyền xong gói tin
                NVIC_SystemReset(); // Lệnh ép lõi ARM Cortex-M4 tái khởi động lập tức
            } else {
                UDS_SendNegativeResponse(sid, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
            }
            break;

        default:
            UDS_SendNegativeResponse(sid, 0x11); // Service Not Supported
            break;
    }

    // Xử lý xong, dọn dẹp bộ đệm chuẩn bị nhận gói tin tiếp theo
    uds_rx_index = 0;
    uds_packet_ready = 0;
}
