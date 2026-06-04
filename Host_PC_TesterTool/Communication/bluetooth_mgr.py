import serial
import time

class BluetoothManager:
    def __init__(self):
        self.ser = None

    def connect(self, port_name, baudrate=115200):
        try:
            # Khởi tạo mở cổng COM với thời gian chờ timeout tránh treo GUI
            self.ser = serial.Serial(port_name, baudrate, timeout=0.2)
            return True
        except Exception as e:
            print(f"Lỗi kết nối Bluetooth: {e}")
            return False

    def disconnect(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    def send_uds_packet(self, packet_bytes):
        if self.ser and self.ser.is_open:
            self.ser.write(packet_bytes)

    def receive_uds_packet(self):
        if not self.ser or not self.ser.is_open:
            return None
            
        # Đọc byte đầu tiên (Byte quy định độ dài toàn gói tin)
        length_byte = self.ser.read(1)
        if not length_byte:
            return None
            
        total_len = length_byte[0]
        if total_len > 0:
            # Đọc nốt phần dung lượng bytes còn lại của gói UDS
            remaining_bytes = self.ser.read(total_len - 1)
            return length_byte + remaining_bytes
        return None

    def read_raw_telemetry(self):
        """Đọc chuỗi Telemetry chu kỳ 100ms dòng lệnh sprintf của Giang bắn lên"""
        if self.ser and self.ser.is_open and self.ser.in_waiting > 0:
            try:
                line = self.ser.readline().decode('utf-8', errors='ignore')
                if line.startswith("Pos="):
                    return line.strip()
            except:
                pass
        return None