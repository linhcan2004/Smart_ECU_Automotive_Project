import asyncio
import threading
import queue
from bleak import BleakClient

class BluetoothManager:
    def __init__(self):
        self.client = None
        self.loop = None
        self.thread = None
        self.rx_queue = queue.Queue() # Hộp thư chờ chứa dữ liệu từ xe gửi lên
        self.mac_address = None
        self.connected = False
        
        # UUID kênh truyền nhận đặc hiệu của con xe cụ cung cấp
        self.UART_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"

    def connect(self, mac_address):
        """ Hàm kết nối đồng bộ gọi từ Giao diện cũ """
        self.mac_address = mac_address
        self.rx_queue = queue.Queue() # Làm sạch hộp thư
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
        """ Mỗi khi xe bắn gói tin về, hàm này tự đút từng byte vào hộp thư """
        for byte in data:
            self.rx_queue.put(byte)

    def read_raw_telemetry(self):
        """ 🌟 HÀM BỔ SUNG: Đọc chuỗi chữ Telemetry thô (main.py đang gọi hàm này) """
        try:
            if self.rx_queue.empty():
                return ""
            
            data_bytes = bytearray()
            # Hút sạch các ký tự chữ đang nằm trong hộp thư chờ ra để dịch thành text
            while not self.rx_queue.empty():
                data_bytes.append(self.rx_queue.get_nowait())
                
            return data_bytes.decode('utf-8', errors='ignore')
        except Exception as e:
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

    async def _async_write(self, data):
        if self.client and self.client.is_connected:
            try:
                # Ghi dữ liệu thô xuống đường ống UUID của xe
                await self.client.write_gatt_char(self.UART_UUID, data)
            except Exception as e:
                print(f"Lỗi gửi gói tin BLE: {e}")

    def receive_uds_packet(self):
        """ Hàm hứng gói tin phản hồi UDS nhị phân """
        try:
            # Bốc byte đầu tiên ra xem độ dài
            first_byte = self.rx_queue.get(timeout=0.5)
            packet = bytearray([first_byte])
            
            total_len = first_byte
            if 0 < total_len < 64:
                for _ in range(total_len - 1):
                    packet.append(self.rx_queue.get(timeout=0.5))
            return bytes(packet)
        except queue.Empty:
            return None