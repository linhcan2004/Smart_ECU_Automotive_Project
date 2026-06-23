import tkinter as tk
import datetime

class DashboardPanel(tk.LabelFrame):
    def __init__(self, parent):
        super().__init__(parent, text=" BẢNG GIÁM SÁT & CẢNH BÁO TRẠNG THÁI ", padx=10, pady=10)
        
        # --- Cột trái: Khu vực Thông số Động học ---
        info_frame = tk.Frame(self)
        info_frame.pack(side="left", fill="both", expand=True)
        
        self.lbl_pos = tk.Label(info_frame, text="Vị trí xe (Center=3500): Tạm dừng", font=("Arial", 11))
        self.lbl_pos.pack(anchor="w", pady=2)
        
        self.lbl_err = tk.Label(info_frame, text="Sai số lệch làn (Error): 0", font=("Arial", 11))
        self.lbl_err.pack(anchor="w", pady=2)
        
        self.lbl_encoder = tk.Label(info_frame, text="Xung Encoder: L=0 | R=0", font=("Arial", 11), fg="blue")
        self.lbl_encoder.pack(anchor="w", pady=2)
        
        # --- Cột phải: Khu vực Nhật ký cảnh báo sự cố ---
        log_frame = tk.LabelFrame(self, text=" Nhật ký Sự kiện & Lỗi ", padx=5, pady=5)
        log_frame.pack(side="right", fill="both", expand=True, padx=10)
        
        # Khung Text để cuộn hiển thị log
        self.txt_log = tk.Text(log_frame, width=45, height=6, state="disabled", bg="#FAFAFA", font=("Consolas", 9))
        self.txt_log.pack(fill="both", expand=True)
        
        # Biến cờ để chống spam log khi lỗi diễn ra liên tục
        self.last_err_state = "NORMAL"

    def log_event(self, msg):
        """Hàm công khai để các module khác (như main.py) có thể bắn thông báo vào đây"""
        self.txt_log.config(state="normal")
        now = datetime.datetime.now().strftime("%H:%M:%S")
        self.txt_log.insert(tk.END, f"[{now}] {msg}\n")
        self.txt_log.see(tk.END) # Tự động cuộn xuống dòng mới nhất
        self.txt_log.config(state="disabled")

    def update_telemetry_ui(self, raw_string):
        """Bóc tách chuỗi Telemetry và phát hiện sự cố tự động"""
        try:
            parts = raw_string.split()
            data = {}
            for p in parts:
                k, v = p.split("=")
                data[k] = v
                
            err_str = data.get('Err', '0')
            self.lbl_pos.config(text=f"Vị trí xe (Center=3500): {data.get('Pos', 'N/A')}")
            self.lbl_err.config(text=f"Sai số lệch làn (Error): {err_str}")
            self.lbl_encoder.config(text=f"Xung Encoder: Bánh Trái={data.get('L', '0')} | Bánh Phải={data.get('R', '0')}")
            
            # --- LOGIC CẢNH BÁO SỰ CỐ LỆCH LÀN ---
            try:
                err_val = int(err_str)
                # Giả sử sai số > 1500 là xe đang văng ra khỏi làn
                if abs(err_val) > 1500 and self.last_err_state == "NORMAL":
                    self.log_event("⚠️ CẢNH BÁO: Xe văng quỹ đạo nghiêm trọng!")
                    self.last_err_state = "ERROR"
                elif abs(err_val) <= 1500 and self.last_err_state == "ERROR":
                    self.log_event("✅ Xe đã quay lại quỹ đạo ổn định.")
                    self.last_err_state = "NORMAL"
            except ValueError:
                pass
                
        except Exception as e:
            # Nếu xe gửi lên một chuỗi chữ thô (như cảnh báo lỗi cảm biến từ chip) thay vì thông số
            if "ERROR" in raw_string.upper() or "WARN" in raw_string.upper():
                self.log_event(f"🔴 SỰ CỐ TỪ ECU: {raw_string.strip()}")