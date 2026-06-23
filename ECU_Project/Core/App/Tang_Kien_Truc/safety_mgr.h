#ifndef SAFETY_MGR_H
#define SAFETY_MGR_H

#include <stdint.h>

// Định nghĩa các ngưỡng an toàn (Tránh dùng Magic Number trong code)
#define GYRO_CRASH_THRESHOLD    25000   // Ngưỡng vận tốc góc báo lật xe
#define CRASH_FILTER_SAMPLES    6       // Số mẫu liên tiếp xác nhận va chạm
#define TASK_HEALTH_FULL_MASK   0x07    // 3 Bit (0,1,2) tương ứng 3 Task (111b)

// Các API công khai cho hệ điều hành gọi
void Safety_Init(void);
void Safety_CheckIMU(void);
void Safety_MonitorTasks(void);
void Safety_EnterSafeState(const char* reason);

#endif /* SAFETY_MGR_H */