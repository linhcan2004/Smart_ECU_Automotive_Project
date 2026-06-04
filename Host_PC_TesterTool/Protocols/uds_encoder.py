import struct

def encode_session_control(session_type=0x03):
    """SID 0x10: Chuyển đổi phiên làm việc (Mặc định chọn Extended Session = 0x03)"""
    return bytes([0x03, 0x10, session_type])

def encode_request_seed():
    """SID 0x27 (Sub 0x01): Gửi yêu cầu xin mã bảo mật ẩn (Seed) từ xe"""
    return bytes([0x03, 0x27, 0x01])

def encode_send_key(seed):
    """SID 0x27 (Sub 0x02): Lấy Seed về tính toán Key theo thuật toán đối xứng và gửi lên xe"""
    # Thuật toán đối xứng bắt buộc phải khớp hoàn toàn với lõi C dưới chip STM32
    key = seed ^ 0x55AA55AA
    # Ép kiểu số nguyên không dấu 4 bytes dạng Little-Endian ('<I') tương thích kiến trúc ARM
    key_bytes = struct.pack('<I', key)
    return bytes([0x07, 0x27, 0x02]) + key_bytes

def encode_read_did(did):
    """SID 0x22: Đọc tham số từ xe bằng Mã định danh DID (2 Bytes)"""
    did_h = (did >> 8) & 0xFF
    did_l = did & 0xFF
    return bytes([0x04, 0x22, did_h, did_l])

def encode_write_pid(kp, ki, kd):
    """SID 0x2E (DID 0x0200): Đóng gói bộ 3 số thực float chỉnh định số vòng kín PID bám làn"""
    # Đóng gói liền mạch 3 biến float (mỗi biến 4 bytes -> tổng 12 bytes) dạng Little-Endian
    pid_bytes = struct.pack('<fff', kp, ki, kd)
    return bytes([0x10, 0x2E, 0x02, 0x00]) + pid_bytes

def encode_write_run_flag(status):
    """SID 0x2E (DID 0x0100): Bật/Tắt cho phép xe chạy (1: Chạy, 0: Phanh dừng)"""
    return bytes([0x05, 0x2E, 0x01, 0x00, 1 if status else 0])

def encode_ecu_reset():
    """SID 0x11: Phát lệnh ép mạch điều khiển trung tâm xe tái khởi động khẩn cấp"""
    return bytes([0x03, 0x11, 0x01])