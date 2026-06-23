import asyncio
import threading
import time
from bleak import BleakClient

class BluetoothManager:
    def __init__(self):
        self.client = None
        self.loop = None
        self.thread = None
        self.rx_buffer = bytearray()  # Bộ nhớ đệm chung chứa dữ liệu nhận được từ xe
        self.rx_lock = threading.Lock()
        self.mac_address = None
        self.connected = False
        
        # UUID kênh truyền nhận đặc biệt của con xe cụ cung cấp
        self.UART_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"

    def connect(self, mac_address):
        """ Hàm kết nối đồng bộ gọi từ Giao diện cũ """
        self.mac_address = mac_address
        with self.rx_lock:
            self.rx_buffer.clear()
        self.connected_event = threading.Event()
        
        # Kích hoạt một luồng chạy ngầm độc lập dành riêng cho Asyncio BLE
        self.thread = threading.Thread(target=self._run_async_loop, daemon=True)
        self.thread.start()
        
        # Chờ tối đa 10 giây xem luồng ngầm kết nối thành công không
        success = self.connected_event.wait(timeout=10.0)
        return success and self.connected

    def _run_async_loop(self):
        """ Luồng ngầm tạo môi trường Async cho Bleak vận hành """
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        try:
            self.loop.run_until_complete(self._async_connect_and_listen())
        except Exception as e:
            print(f"Lỗi luồng ngầm BLE: {e}")
        finally:
            self.connected = False

    async def _async_connect_and_listen(self):
        """ Hàm lõi kết nối BLE bất đồng bộ """
        try:
            print(f"Đang dùng Bleak kết nối tới xe qua BLE MAC: {self.mac_address}...")
            self.client = BleakClient(self.mac_address)
            await self.client.connect()
            self.connected = True
            print("▶ Kết nối Bluetooth qua BLE THÀNH CÔNG!")
            
            # Kích hoạt chế độ Notify (Hứng dữ liệu chủ động từ xe bắn về)
            await self.client.start_notify(self.UART_UUID, self._handle_ble_rx)
            
            # Báo cho giao diện biết: Kết nối thành công rồi!
            self.connected_event.set()
            
            # Giữ cho luồng sống liên tục để nghe dữ liệu cho đến khi ngắt kết nối
            while self.connected and self.client.is_connected:
                await asyncio.sleep(0.1)
                
        except Exception as e:
            print(f"❌ Không thể kết nối BLE phần cứng: {e}")
            self.connected = False
            self.connected_event.set()

    def _handle_ble_rx(self, sender, data):
        """ Mỗi khi xe bắn gói tin về, hàm này đệm dữ liệu vào bộ nhớ chung """
        with self.rx_lock:
            self.rx_buffer.extend(data)

    def read_raw_telemetry(self):
        """ 🌟 HÀM BỔ SUNG: Đọc chuỗi chữ Telemetry thô (main.py đang gọi hàm này) """
        try:
            with self.rx_lock:
                # Nhìn nhanh xem có chuỗi Telemetry trong bộ nhớ đệm không
                if not self.rx_buffer:
                    return ""
                data = bytes(self.rx_buffer)

            # Chỉ cố gắng giải mã chuỗi Telemetry bằng UTF-8, nếu không phải Telemetry thì bỏ qua
            text = data.decode('utf-8', errors='ignore')
            lines = text.splitlines()
            for line in reversed(lines):
                if line.startswith("Pos="):
                    return line.strip()
            return ""
        except Exception:
            return ""

    def disconnect(self):
        """ Ngắt kết nối an toàn """
        self.connected = False
        if self.loop and self.loop.is_running():
            asyncio.run_coroutine_threadsafe(self._async_disconnect(), self.loop)

    async def _async_disconnect(self):
        if self.client:
            try:
                await self.client.stop_notify(self.UART_UUID)
                await self.client.disconnect()
                print("Đã ngắt sóng BLE an toàn.")
            except:
                pass

    def send_uds_packet(self, packet_bytes):
        """ Hàm gửi lệnh UDS từ giao diện xuống xe """
        if self.connected and self.loop:
            # Chuyển lệnh đồng bộ từ GUI thành lệnh async phóng xuống chip
            asyncio.run_coroutine_threadsafe(self._async_write(packet_bytes), self.loop)

    def receive_uds_packet(self, timeout=2.5):
        """ Hàm hứng gói tin phản hồi UDS nhị phân (Đã thêm Global Timeout và chống nhiễu) """
        start_time = time.time()
        try:
            while time.time() - start_time < timeout:
                packet = self._try_extract_uds_packet()
                if packet:
                    return packet
                time.sleep(0.05)
        except Exception as e:
            print(f"Lỗi khi đọc UDS: {e}")
        return None

    def _try_extract_uds_packet(self):
        """Cố gắng trích gói UDS hợp lệ từ bộ đệm nhận chung"""
        with self.rx_lock:
            if len(self.rx_buffer) < 2:
                return None
            total_len = self.rx_buffer[0]
            if total_len <= 1 or total_len > len(self.rx_buffer):
                return None
            packet = bytes(self.rx_buffer[:total_len])
            sid = packet[1]
            valid_sids = [0x7F, 0x50, 0x67, 0x62, 0x6E, 0x51]
            if sid in valid_sids:
                del self.rx_buffer[:total_len]
                return packet
            del self.rx_buffer[0]
        return None

    async def _async_write(self, data):
        if self.client and self.client.is_connected:
            try:
                # Ghi dữ liệu thô xuống đường ống UUID của xe
                await self.client.write_gatt_char(self.UART_UUID, data)
            except Exception as e:
                print(f"Lỗi gửi gói tin BLE: {e}")
