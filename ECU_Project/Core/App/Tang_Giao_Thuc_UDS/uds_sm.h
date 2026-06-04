#ifndef UDS_SM_H_
#define UDS_SM_H_

#include "main.h"

// Định nghĩa các Dịch vụ UDS (Diagnostic Service Identifiers - SID)
#define UDS_SID_DIAG_SESSION_CONTROL    0x10
#define UDS_SID_ECU_RESET               0x11
#define UDS_SID_READ_DATA_BY_ID         0x22
#define UDS_SID_SECURITY_ACCESS         0x27
#define UDS_SID_WRITE_DATA_BY_ID        0x2E

// Định nghĩa mã lỗi UDS chuẩn (Negative Response Codes - NRC)
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED   0x12
#define UDS_NRC_INCORRECT_MESSAGE_LENGTH    0x13
#define UDS_NRC_CONDITIONS_NOT_CORRECT      0x22
#define UDS_NRC_REQUEST_SEQUENCE_ERROR      0x24
#define UDS_NRC_SECURITY_ACCESS_DENIED      0x33
#define UDS_NRC_INVALID_KEY                 0x35

// Định nghĩa các Mã định danh dữ liệu (Data Identifiers - DID)
#define UDS_DID_RUN_FLAG                0x0100  // Đọc/Ghi trạng thái chạy của xe (1 byte)
#define UDS_DID_PID_GAINS               0x0200  // Đọc/Ghi bộ 3 hệ số Kp, Ki, Kd (12 bytes float)
#define UDS_DID_ENCODER_DATA            0x0300  // Chỉ đọc số xung 2 bánh xe (8 bytes)
#define UDS_DID_LINE_ERROR              0x0400  // Chỉ đọc sai số lệch làn hiện tại (2 bytes)

// Các hàm điều khiển hệ thống UDS
void UDS_Init(void);
void UDS_Process(void);

#endif
