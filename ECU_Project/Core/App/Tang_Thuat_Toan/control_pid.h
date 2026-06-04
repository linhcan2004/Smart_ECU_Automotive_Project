#ifndef CONTROL_PID_H_
#define CONTROL_PID_H_

#include "main.h"

// Khai báo các hàm điều khiển chuyển động của xe
void Reset_PID(void);
void Motor_Drive(int left_speed, int right_speed);

#endif
