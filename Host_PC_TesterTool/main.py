import tkinter as tk
from Communication.bluetooth_mgr import BluetoothManager
from GUI.dashboard import DashboardPanel
from GUI.tester_tool import TesterToolPanel

class AutomotiveApp:
    def __init__(self, window):
        self.window = window
        self.window.title("HỆ THỐNG GIÁM SÁT & CHẨN ĐOÁN ECU Ô TÔ SMART LINE-TRACKING - VNU UET 2026")
        self.window.geometry("680x580")
        
        # Khởi tạo lõi kết nối phần cứng
        self.bt = BluetoothManager()
        
        # Tạo khu vực kết nối nhanh cổng COM
        conn_frame = tk.Frame(self.window, pady=10)
        conn_frame.pack(fill="x")
        tk.Label(conn_frame, text="Nhập địa chỉ MAC HC-06:", font=("Consolas", 11)).pack(side="left", padx=10)
        self.ent_port = tk.Entry(conn_frame, width=10)
        self.ent_port.insert(0, "AF:6C:03:4F:EB:DB")
        self.ent_port.pack(side="left", padx=5)
        
        self.btn_connect = tk.Button(conn_frame, text="KẾT NỐI XE", bg="green", fg="white", command=self.toggle_connection)
        self.btn_connect.pack(side="left", padx=10)

        # Nhúng 2 Panel giao diện đồ họa đã thiết kế từ thư mục GUI
        self.pnl_dashboard = DashboardPanel(self.window)
        self.pnl_dashboard.pack(fill="both", expand=True, padx=15, pady=5)
        
        self.pnl_tester = TesterToolPanel(self.window, self.bt)
        self.pnl_tester.pack(fill="both", expand=True, padx=15, pady=10)
        
        self.is_connected = False

    def toggle_connection(self):
        if not self.is_connected:
            port = self.ent_port.get()
            self.pnl_dashboard.log_event(f"⏳ Đang kết nối tới MAC: {port}...")
            
            if self.bt.connect(port):
                self.is_connected = True
                self.btn_connect.config(text="NGẮT KẾT NỐI", bg="red")
                self.pnl_dashboard.log_event("🟢 Đã kết nối Bluetooth thành công!")
                self.listen_telemetry_loop()
            else:
                self.pnl_dashboard.log_event("❌ Lỗi: Không thể kết nối tới xe!")
                tk.messagebox.showerror("Thất bại", f"Không thể mở kết nối {port}.")
        else:
            self.bt.disconnect()
            self.is_connected = False
            self.btn_connect.config(text="KẾT NỐI XE", bg="green")
            self.pnl_dashboard.log_event("⚪ Đã ngắt kết nối an toàn.")

    def listen_telemetry_loop(self):
        """Vòng lặp ngầm liên tục quét kiểm tra dữ liệu từ xe gửi lên"""
        if self.is_connected:
            raw_line = self.bt.read_raw_telemetry()
            if raw_line:
                # Nếu bắt được dòng dữ liệu Telemetry chu kỳ 100ms, cập nhật lên giao diện ngay
                self.pnl_dashboard.update_telemetry_ui(raw_line)
            
            # Đặt lịch hẹn sau 30ms quay lại hàm này quét tiếp, giải phóng CPU giữ cho GUI mượt mà
            self.window.after(30, self.listen_telemetry_loop)

if __name__ == "__main__":
    root = tk.Tk()
    app = AutomotiveApp(root)
    root.mainloop()