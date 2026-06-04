import tkinter as tk

class DashboardPanel(tk.LabelFrame):
    def __init__(self, parent):
        super().__init__(parent, text=" BẢNG GIÁM SÁT ĐỘNG HỌC XE (TELEMETRY) ", padx=10, pady=10)
        
        # Thiết kế các nhãn hiển thị thông số nhanh
        self.lbl_pos = tk.Label(self, text="Vị trí xe (Center=3500): Tạm dừng", font=("Arial", 11))
        self.lbl_pos.pack(anchor="w", pady=2)
        
        self.lbl_err = tk.Label(self, text="Sai số lệch làn (Error): 0", font=("Arial", 11))
        self.lbl_err.pack(anchor="w", pady=2)
        
        self.lbl_encoder = tk.Label(self, text="Xung Encoder: L=0 | R=0", font=("Arial", 11), fg="blue")
        self.lbl_encoder.pack(anchor="w", pady=2)

    def update_telemetry_ui(self, raw_string):
        """Bóc tách chuỗi 'Pos=X Err=Y PID=Z L=A R=B Gyro=C' đưa lên màn hình"""
        try:
            parts = raw_string.split()
            data = {}
            for p in parts:
                k, v = p.split("=")
                data[k] = v
                
            self.lbl_pos.config(text=f"Vị trí xe (Center=3500): {data.get('Pos', 'N/A')}")
            self.lbl_err.config(text=f"Sai số lệch làn (Error): {data.get('Err', 'N/A')}")
            self.lbl_encoder.config(text=f"Xung Encoder: Bánh Trái={data.get('L', '0')} | Bánh Phải={data.get('R', '0')}")
        except:
            pass