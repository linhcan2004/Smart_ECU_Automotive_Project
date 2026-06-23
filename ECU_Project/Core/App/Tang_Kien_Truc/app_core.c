#include "app_core.h"
#include "app_bsp.h"
#include "mpu6500.h"
#include "line_sensor.h"
#include "control_pid.h"
#include <stdio.h>
#include <string.h>

// Cấp phát vật lý các biến hệ thống chính
volatile uint8_t run_flag = 0;
volatile uint8_t inject_error = 0;
char uart_buf[150] = {0};
uint32_t last_uart_time = 0;
volatile uint8_t task_health_mask = 0;

// Cấu hình kích thước bộ nhớ tĩnh FreeRTOS
#define CRITICAL_STACK_SIZE    128
#define CONTROL_STACK_SIZE     512
#define DIAGNOSTIC_STACK_SIZE  256
#define QUEUE_LEN              4
#define QUEUE_ITEM_SIZE        sizeof(int)

osThreadId_t criticalTaskHandle;
osThreadId_t controlTaskHandle;
osThreadId_t diagnosticTaskHandle;
osMutexId_t uartMutexHandle;
osMessageQueueId_t controlQueueHandle;

StackType_t xCriticalTaskStack[CRITICAL_STACK_SIZE];
StackType_t xControlTaskStack[CONTROL_STACK_SIZE];
StackType_t xDiagnosticTaskStack[DIAGNOSTIC_STACK_SIZE];
StaticTask_t xCriticalTaskBuffer;
StaticTask_t xControlTaskBuffer;
StaticTask_t xDiagnosticTaskBuffer;

uint8_t controlQueueBuffer[QUEUE_LEN * QUEUE_ITEM_SIZE];
StaticQueue_t xStaticQueueBuffer;

const osMutexAttr_t uartMutex_attributes = { "uartMutex", osMutexPrioInherit, NULL, 0 };
const osMessageQueueAttr_t controlQueue_attributes = {
    .name = "ControlQueue", .cb_mem = &xStaticQueueBuffer, .cb_size = sizeof(xStaticQueueBuffer),
    .mq_mem = &controlQueueBuffer[0], .mq_size = sizeof(controlQueueBuffer)
};

static void StartCriticalTask(void *argument);
static void StartControlTask(void *argument);
static void StartDiagnosticTask(void *argument);

void App_Core_Init(void) {
     // MPU6500_Init();
    uartMutexHandle = osMutexNew(&uartMutex_attributes);
    controlQueueHandle = osMessageQueueNew(QUEUE_LEN, QUEUE_ITEM_SIZE, &controlQueue_attributes);

    const osThreadAttr_t critical_attributes = {
        .name = "CriticalTask", .cb_mem = &xCriticalTaskBuffer, .cb_size = sizeof(xCriticalTaskBuffer),
        .stack_mem = &xCriticalTaskStack[0], .stack_size = sizeof(xCriticalTaskStack),
        .priority = (osPriority_t) osPriorityRealtime,
    };
    criticalTaskHandle = osThreadNew(StartCriticalTask, NULL, &critical_attributes);

    const osThreadAttr_t control_attributes = {
        .name = "ControlTask", .cb_mem = &xControlTaskBuffer, .cb_size = sizeof(xControlTaskBuffer),
        .stack_mem = &xControlTaskStack[0], .stack_size = sizeof(xControlTaskStack),
        .priority = (osPriority_t) osPriorityNormal,
    };
    controlTaskHandle = osThreadNew(StartControlTask, NULL, &control_attributes);

    const osThreadAttr_t diagnostic_attributes = {
        .name = "DiagnosticTask", .cb_mem = &xDiagnosticTaskBuffer, .cb_size = sizeof(xDiagnosticTaskBuffer),
        .stack_mem = &xDiagnosticTaskStack[0], .stack_size = sizeof(xDiagnosticTaskStack),
        .priority = (osPriority_t) osPriorityBelowNormal,
    };
    diagnosticTaskHandle = osThreadNew(StartDiagnosticTask, NULL, &diagnostic_attributes);
}

// TÁC VỤ 1: CRITICAL TASK (Chu kỳ nghiêm ngặt 5ms - An toàn khẩn cấp)
volatile CarState_t car_state = CAR_STATE_IDLE; // Cấp phát vật lý biến trạng thái xe
char critical_buf[64]; // Bộ đệm biệt lập riêng cho ngắt khẩn cấp
void StartCriticalTask(void *argument) {
    uint32_t tickIncrement = pdMS_TO_TICKS(5);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t failsafe_count = 0; // Bộ đếm xác nhận liên tiếp

    for(;;) {
        if (car_state == CAR_STATE_RUNNING) {
            int16_t gyro_z = Read_Gyro_Z() - gyro_z_offset;

            if (gyro_z > 20000 || gyro_z < -20000) {
                failsafe_count++;
                // Chỉ kích hoạt khi 3 mẫu LIÊN TIẾP đều vượt ngưỡng (15ms)
                // Loại bỏ hoàn toàn nhiễu đơn lẻ
                if (failsafe_count >= 3) {
                    Motor_Drive(0, 0);
                    // dừng motor
                    run_flag = 0;
                    failsafe_count = 0;
                    sprintf(uart_buf, "[CRITICAL] FAILSAFE! gyro=%d\r\n", gyro_z);
                    HAL_UART_Transmit(&huart6, (uint8_t*)uart_buf, strlen(uart_buf), 50);
                }
            } else {
                failsafe_count = 0; // Reset nếu mẫu bình thường
            }
        }

        task_health_mask |= (1 << 0);
        vTaskDelayUntil(&xLastWakeTime, tickIncrement);
    }
}

// TÁC VỤ 2: CONTROL TASK (Chu kỳ 20ms - Điều khiển Động học xe & PID)
void StartControlTask(void *argument) {
    uint32_t tickIncrement = pdMS_TO_TICKS(20);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Khai báo mảng dữ liệu lịch sử ngã rẽ dạng biến nội bộ Tác vụ để đảm bảo an toàn luồng
    static int hist_left[5]  = {0, 0, 0, 0, 0};
    static int hist_right[5] = {0, 0, 0, 0, 0};
    int received_speed = 0; // Biến tạm lưu tốc độ đọc ra từ Queue


    for(;;) {
    	// KHỐI GIẢ LẬP LỖI (FAILURE INJECTION PORT)
    	if (inject_error == 1) {
    	// Ép tác vụ rơi vào vòng lặp vô hạn, mô phỏng lỗi treo thuật toán PID
    	while(1) {
    	// CPU bị giữ chặt ở đây, không thể thoát ra ngoài
    		}
    	}

    	//  Kiểm tra xem có lệnh thay đổi tốc độ từ UDS/Diagnostic gửi qua không
    	// Sử dụng timeout = 0 (osNoWait) để không làm block chu kỳ tính toán PID 20ms nghiêm ngặt
    	if (osMessageQueueGet(controlQueueHandle, &received_speed, NULL, 0) == osOK) {
    		// Nếu nhận được giá trị hợp lệ từ Queue, cập nhật ngay vận tốc mong muốn mới cho xe
    		if (received_speed >= 0 && received_speed <= max_speed) {
    			base_speed = received_speed;
    		}
    	}

        if (run_flag == 1) {

            // KHỐI KHỞI ĐẦU: TỰ ĐỘNG HỌC SA BÀN (Auto Calibration)
            if (is_calibrated == 0) {
            	car_state = CAR_STATE_CALIBRATE;
                Auto_Calibration();
                is_calibrated = 1;
                run_flag = 0;
                car_state = CAR_STATE_IDLE;
                Reset_PID();
                continue;
            } else {

            Refresh_ADC();
            int position = Read_Position();
            if (position != -1) {
                // Đẩy lùi lịch sử trạng thái (Shift Register)
                for (int i = 4; i > 0; i--) {
                    hist_left[i]  = hist_left[i - 1];
                    hist_right[i] = hist_right[i - 1];
                }
                // Ghi nhận số mắt phát hiện vạch đen
                hist_right[0] = (adc_values[0] > 3500) + (adc_values[1] > 3500) + (adc_values[2] > 3500);
                hist_left[0]  = (adc_values[5] > 3500) + (adc_values[6] > 3500) + (adc_values[7] > 3500);
            }

            // KHỐI XỬ LÝ NGÃ RẼ (Đã thay thế hoàn toàn HAL_Delay bằng osDelay để nhường CPU)
            if (position == -1) {
            	car_state = CAR_STATE_TURNING;
                int total_left  = hist_left[2] + hist_left[3] + hist_left[4];
                int total_right = hist_right[2] + hist_right[3] + hist_right[4];
                int turn_direction = (total_left > total_right) ? -1 : ((total_right > total_left) ? 1 : -1);

                int turn_speed = base_speed * 0.5;
                if (turn_speed < 200) turn_speed = 200;
                sprintf(uart_buf, "TURN: base=%d turn_speed=%d dir=%d\r\n",
                            base_speed, turn_speed, turn_direction);
                HAL_UART_Transmit(&huart6, (uint8_t*)uart_buf, strlen(uart_buf), 50);

                if (turn_direction == -1) Motor_Drive(-turn_speed, turn_speed); // Xoay compa trái
                else                      Motor_Drive(turn_speed, -turn_speed); // Xoay compa phải

                osDelay(30); // osDelay giúp nhường quyền CPU cho tác vụ khác thay vì đứng chết đóng băng chip

                uint8_t found_line_spin = 0; // Cờ kiểm soát bẫy lỗi ngã rẽ
                uint32_t spin_start = HAL_GetTick();

                while (HAL_GetTick() - spin_start < 3000) {
                	task_health_mask |= (1 << 1);
                    Refresh_ADC();
                    // Điều kiện bắt lại tâm vạch đường mới
                    if ((adc_values[2] > 3500 && adc_values[3] > 3500) || (adc_values[4] > 3500 && adc_values[5] > 3500)) {
                        if (turn_direction == 1) Motor_Drive(-turn_speed * 0.5, turn_speed * 0.5);
                        else                      Motor_Drive(turn_speed * 0.5, -turn_speed * 0.5);
                        osDelay(45);

                        Motor_Drive(0, 0);
                        osDelay(80);
                        found_line_spin = 1;
                        break;
                    }
                    osDelay(2); // Tránh loop cạn kiệt CPU của task thấp hơn
                }
                Reset_PID();
                for(int k = 0; k < 5; k++) {
                	hist_left[k] = 0; hist_right[k] = 0;
                	}

                if (found_line_spin == 1) {
                                        car_state = CAR_STATE_RUNNING; // Bắt line thành công -> Kích hoạt lại Failsafe đường thẳng
                                    } else {
                                        Motor_Drive(0, 0);
                                        run_flag = 0;
                                        car_state = CAR_STATE_IDLE;    // Thất bại -> Dừng xe khẩn cấp, đứng im chờ cứu hộ
                                    }

                } else {
						// KHỐI THUẬT TOÁN PID BÁM LINE ĐƯỜNG THẲNG
                		car_state = CAR_STATE_RUNNING;
						error = position - 3500;
						int current_base = (error > 2500 || error < -2500) ? (int)(base_speed * 0.7f) : base_speed;

						P  = error;
						I += error;
						if (I >  4000) I =  4000;
						if (I < -4000) I = -4000;

						int raw_D = error - last_error;
						if (raw_D > 800) raw_D = 800;
						if (raw_D < -800) raw_D = -800;

						filtered_D = (0.4f * (float)raw_D) + (0.6f * filtered_D);
						PID_value = (int)((Kp * (float)P) + (Ki * (float)I) + (Kd * filtered_D));
						last_error = error;

						int max_pid = current_base + 150;
						if (PID_value >  max_pid) PID_value =  max_pid;
						if (PID_value < -max_pid) PID_value = -max_pid;

						int final_left_speed = current_base - PID_value;
						int final_right_speed = current_base + PID_value;

						if (final_left_speed < -40) final_left_speed = -40;
						if (final_right_speed < -40) final_right_speed = -40;

						Motor_Drive(final_left_speed, final_right_speed);
                }
            }
        } else {
        	car_state = CAR_STATE_IDLE;
            Motor_Drive(0, 0);
        }

        // ĐIỂM DANH: Bật Bit 1 (0x02) chứng minh Task 20ms tính PID không bị treo
        task_health_mask |= (1 << 1);
        vTaskDelayUntil(&xLastWakeTime, tickIncrement);
    }
}

// TÁC VỤ 3: DIAGNOSTIC TASK (Chu kỳ 100ms - Đo lường & Chẩn đoán lỗi UDS)
void StartDiagnosticTask(void *argument) {
    uint32_t tickIncrement = pdMS_TO_TICKS(100);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    uint32_t simulation_timer = 0;
	int simulated_target_speed = 450;
    for(;;) {
        // Đọc dữ liệu phản hồi từ phần cứng xe cũ
        enc_left_count  = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
        enc_right_count = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
        int16_t gyro_p = Read_Gyro_Z() - gyro_z_offset;
        int position = Read_Position();

        // CHIẾM ĐỘC CHIẾM KHÓA MUTEX TRƯỚC KHI XUẤT ĐỮ LIỆU QUA CỔNG SERIAL
        if (osMutexAcquire(uartMutexHandle, osWaitForever) == osOK) {

            // 1. Đóng gói gửi gói tin giám sát Telemetry lên máy tính Dashboard
            sprintf(uart_buf, "Pos=%d Err=%d PID=%d L=%ld R=%ld Gyro=%d\r\n",
                    position, error, PID_value, enc_left_count, enc_right_count, gyro_p);
            HAL_UART_Transmit(&huart6, (uint8_t*)uart_buf, strlen(uart_buf), 60);

            // 2.Thành viên 02 sau này sẽ nhúng lõi phân tích cú pháp lệnh UDS ISO 14229 tại đây để nhận lệnh chẩn đoán từ máy tính và trả về dữ liệu tương ứng qua UART.
            // ...

            osMutexRelease(uartMutexHandle); // Giải phóng khóa sau khi truyền tải xong
        }

        // KHỐI GIẢ LẬP GỬI LỆNH QUA QUEUE ( TEST THỬ HẠ TẦNG)
        simulation_timer += 100; // Tăng dần theo chu kỳ task 100ms
        if (simulation_timer >= 5000) { // Cứ sau 5000ms (5 giây)
        	simulation_timer = 0;
        	// Đổi luân phiên tốc độ giả lập để xem Tác vụ điều khiển thay đổi theo
        	simulated_target_speed = (simulated_target_speed == 450) ? 300 : 450;

        	// Đẩy vận tốc giả lập vào Hàng đợi chuyển tiếp sang Control Task
        	osMessageQueuePut(controlQueueHandle, &simulated_target_speed, 0, 0);
        }

        // ĐIỂM DANH: Bật Bit 2 (0x04) chứng minh Task 100ms vẫn xử lý truyền thông tốt
        task_health_mask |= (1 << 2);

        // Kiểm tra xem mặt nạ đã tụ hội đủ 3 cờ (Bit 0, Bit 1, Bit 2 -> 0x07) chưa
        if (task_health_mask == 0x07) {
        	HAL_IWDG_Refresh(&hiwdg);  // Thực hiện lệnh "Nuôi chó" - gỡ bom phần cứng an toàn
        	task_health_mask = 0;     // Xóa mặt nạ về 0 để các Task làm lại chu kỳ điểm danh mới
        } else {
        // Nếu thiếu bất kỳ cờ nào (có Task bị treo ngầm)=> KHÔNG NUÔI CHÓ.
        // Trong vòng tối đa 500ms, IWDG phần cứng sẽ tự động ÉP ECU RESET để cứu xe.
        }
        vTaskDelayUntil(&xLastWakeTime, tickIncrement);
    }
}
// Giả sử mảng sensor[8] chứa giá trị số (0 hoặc 1) của 8 mắt đọc
printf("Pos=%d Err=%d SENSORS=%d%d%d%d%d%d%d%d\n", 
       position, 
       error, 
       sensor[0], sensor[1], sensor[2], sensor[3], 
       sensor[4], sensor[5], sensor[6], sensor[7]);