Cấu trúc thư mục của dự án

```text
Automotive_Root/
├── .gitignore                     
├── README.md              
│
├── ECU_Project/                   <-- 1. THƯ MỤC DỰ ÁN NHÚNG STM32 (Trong STM32CubeIDE)
│   ├── Includes/                   <-- (Mặc định) Thư mục ảo quản lý đường dẫn của IDE
│   ├── Drivers/                    <-- (Mặc định) Tầng MCAL thô điều khiển chip của hãng ST
│   │   ├── CMSIS/
│   │   └── STM32F4xx_HAL_Driver/
│   ├── Core/                       <-- Thư mục chứa code chính của dự án
│   │   ├── Inc/
│   │   │   └── main.h              <-- (Mặc định) File cấu hình định nghĩa chân chip
│   │   ├── Src/
│   │   │   ├── main.c              <-- (Mặc định) File gốc khởi động chip
│   │   │   └── stm32f4xx_it.c      <-- (Mặc định) File quản lý các hàm ngắt phần cứng
│   │   ├── Startup/                <-- (Mặc định) Mã nguồn Assembly khởi động nguồn chip
│   │   └── App/                    <-- KHÔNG GIAN LẬP TRÌNH CHÍNH CỦA 3 THÀNH VIÊN NHÓM
│   │       ├── Common/             <-- Tầng kết nối dữ liệu chung giữa các thành viên
│   │       │   └── app_bsp.h       <-- Định nghĩa Struct dữ liệu xe, Khóa Mutex, Hàng đợi Queue
│   │       │
│   │       ├── Tang_Kien_Truc/    <-- PHẦN VIỆC CỦA THÀNH VIÊN 01 (Kiến trúc & An toàn)
│   │       │   ├── app_core.h      <-- Khai báo các Task FreeRTOS
│   │       │   ├── app_core.c      <-- Viết logic khởi chạy các Task FreeRTOS mẫu
│   │       │   ├── mpu6500.h       <-- Khai báo các hàm đọc cảm biến quán tính IMU
│   │       │   ├── mpu6500.c       <-- Viết code thô giao tiếp đọc dữ liệu từ chip MPU6500
│   │       │   ├── safety_mgr.h    <-- Khai báo hàm giám sát an toàn và Watchdog
│   │       │   └── safety_mgr.c    <-- Viết logic chống lật xe và xóa bộ đếm Watchdog
│   │       │
│   │       ├── Tang_Giao_Thuc_UDS/ <-- PHẦN VIỆC CỦA THÀNH VIÊN 02 (Chẩn đoán lỗi UDS)
│   │       │   ├── uds_sm.h        <-- Khai báo máy trạng thái UDS và cấu trúc byte dịch vụ
│   │       │   ├── uds_sm.c        <-- Viết logic xử lý dịch vụ 0x10, bảo mật 0x27 Seed & Key
│   │       │   ├── flash_mem.h     <-- Khai báo các hàm tương tác bộ nhớ Flash
│   │       │   └── flash_mem.c     <-- Viết code đọc/ghi số VIN, số Km tích lũy vào Flash
│   │       │
│   │       └── Tang_Thuat_Toan/    <-- PHẦN VIỆC CỦA THÀNH VIÊN 03 (Thuật toán & Điều khiển)
│   │           ├── control_pid.h   <-- Khai báo cấu trúc bộ điều khiển PID bám làn và giữ tốc độ
│   │           ├── control_pid.c   <-- Viết công thức toán PID tính toán xung PWM xuất ra động cơ
│   │           ├── encoder.h       <-- Khai báo hàm đọc xung bánh xe
│   │           ├── encoder.c       <-- Viết code đọc Timer ở chế độ Encoder Mode để tính vận tốc
│   │           ├── line_sensor.h   <-- Khai báo hàm đọc mắt hồng ngoại
│   │           └── line_sensor.c   <-- Viết code đọc trạng thái dãy 8 cảm biến hồng ngoại
│   └── ECU_Project.ioc            <-- (Mặc định) File giao diện cấu hình chân chip
│
└── Host_PC_TesterTool/             <-- 2. THƯ MỤC PHẦN MỀM PYTHON (Chạy trên máy tính)
    ├── main.py                     <-- File kích hoạt chạy chính của Python
    ├── GUI/                        <-- Thư mục chứa giao diện đồ họa (Thành viên 04 phụ trách)
    │   ├── dashboard.py            <-- Vẽ cụm đồng hồ lái, kim vận tốc, vòng tua máy
    │   └── tester_tool.py          <-- Vẽ giao diện bảng nút bấm gửi lệnh chẩn đoán UDS
    ├── Communication/              <-- Thư mục chứa code kết nối không dây
    │   └── bluetooth_mgr.py        <-- Lập trình luồng chạy ngầm kết nối Bluetooth Socket với xe
    └── Protocols/                  <-- Thư mục chứa code mã hóa giao thức
        ├── uds_encoder.py          <-- Dịch nút bấm thành các chuỗi mã byte UDS (0x10, 0x27...)
        └── uds_decoder.py          <-- Giải mã gói tin từ xe gửi lên và thuật toán Seed & Key
```
