import tkinter as tk
from tkinter import messagebox
from Protocols import uds_encoder as enc
from Protocols import uds_decoder as dec

class TesterToolPanel(tk.LabelFrame):
    def __init__(self, parent, bt_mgr):
        super().__init__(parent, text=" THIẾT BỊ CHẨN ĐOÁN TIÊU CHUẨN UDS (ISO 14229) ", padx=10, pady=10)
        self.bt = bt_mgr
        self.saved_seed = None
        
        # 1. Khu vực quản lý Phiên Bảo Mật (Session & Security)
        sec_frame = tk.Frame(self)
        sec_frame.pack(fill="x", pady=5)
        
        tk.Button(sec_frame, text="1. Bật Extended Session (0x10)", command=self.req_extended).grid(row=0, column=0, padx=5)
        tk.Button(sec_frame, text="2. Xin mã bảo mật Seed (0x27)", command=self.req_seed).grid(row=0, column=1, padx=5)
        tk.Button(sec_frame, text="3. Gửi Key mở khóa ECU", command=self.send_key).grid(row=0, column=2, padx=5)

        # 2. Khu vực tinh chỉnh hệ số điều khiển PID bám làn trực tiếp
        pid_frame = tk.LabelFrame(self, text=" Cân chỉnh hệ số PID bám làn (Ghi DID 0x0200) ")
        pid_frame.pack(fill="x", pady=10)
        
        tk.Label(pid_frame, text="Kp:").grid(row=0, column=0, padx=2)
        self.ent_kp = tk.Entry(pid_frame, width=8); self.ent_kp.insert(0, "0.28"); self.ent_kp.grid(row=0, column=1, padx=5)
        
        tk.Label(pid_frame, text="Ki:").grid(row=0, column=2, padx=2)
        self.ent_ki = tk.Entry(pid_frame, width=8); self.ent_ki.insert(0, "0.001"); self.ent_ki.grid(row=0, column=3, padx=5)
        
        tk.Label(pid_frame, text="Kd:").grid(row=0, column=4, padx=2)
        self.ent_kd = tk.Entry(pid_frame, width=8); self.ent_kd.insert(0, "0.65"); self.ent_kd.grid(row=0, column=5, padx=5)
        
        tk.Button(pid_frame, text="Nạp PID xuống xe (0x2E)", bg="orange", command=self.write_pid_param).grid(row=0, column=6, padx=10, pady=5)

    def req_extended(self):
        self.bt.send_uds_packet(enc.encode_session_control())
        res = self.bt.receive_uds_packet()
        print("UDS Response:", dec.decode_uds_response(res) if res else "No response")

    def req_seed(self):
        self.bt.send_uds_packet(enc.encode_request_seed())
        res = self.bt.receive_uds_packet()
        if res:
            parsed = dec.decode_uds_response(res)
            if parsed.get("service") == "SEED":
                self.saved_seed = parsed["value"]
                messagebox.showinfo("UDS Security", f"Nhận Seed thành công từ xe: {hex(self.saved_seed)}")

    def send_key(self):
        if self.saved_seed is None:
            messagebox.showwarning("Cảnh báo", "Vui lòng xin mã Seed trước!")
            return
        self.bt.send_uds_packet(enc.encode_send_key(self.saved_seed))
        res = self.bt.receive_uds_packet()
        if res:
            parsed = dec.decode_uds_response(res)
            if parsed.get("service") == "SECURITY_UNLOCKED":
                messagebox.showinfo("Thành công", "ECU Đã được bẻ khóa bảo mật thành công!")

    def write_pid_param(self):
        try:
            kp = float(self.ent_kp.get())
            ki = float(self.ent_ki.get())
            kd = float(self.ent_kd.get())
            self.bt.send_uds_packet(enc.encode_write_pid(kp, ki, kd))
            res = self.bt.receive_uds_packet()
            if res:
                parsed = dec.decode_uds_response(res)
                if parsed.get("status") == "SUCCESS":
                    messagebox.showinfo("Đã khắc Flash", "Hệ số PID mới đã được ghi nạp vĩnh viễn vào ô nhớ Flash chip STM32!")
                else:
                    messagebox.showerror("Bị chặn lỗi", f"Ghi thất bại. Mã NRC lỗi từ xe: {parsed.get('nrc')}\n(Nhắc nhở: Cậu đã mở khóa bảo mật bước 1-2-3 chưa?)")
        except ValueError:
            messagebox.showerror("Lỗi dữ liệu", "Hệ số PID nhập vào bắt buộc phải là dạng số thực!")