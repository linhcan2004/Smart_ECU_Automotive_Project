import tkinter as tk
from tkinter import messagebox
from Protocols import uds_encoder as enc
from Protocols import uds_decoder as dec

class TesterToolPanel(tk.LabelFrame):
    def __init__(self, parent, bt_mgr):
        super().__init__(parent, text=" THIẾT BỊ CHẨN ĐOÁN TIÊU CHUẨN UDS (ISO 14229) ", padx=10, pady=10)
        self.bt = bt_mgr
        self.saved_seed = None
        
        # --- KHU VỰC 1: QUẢN LÝ PHIÊN & BẢO MẬT (SESSION & SECURITY) ---
        sec_frame = tk.LabelFrame(self, text=" 1. Phiên làm việc & Bảo mật truy cập ", padx=5, pady=5)
        sec_frame.pack(fill="x", pady=5)
        
        tk.Button(sec_frame, text="1. Bật Extended Session (0x10)", bg="#E3F2FD", command=self.req_extended).grid(row=0, column=0, padx=5, pady=5)
        tk.Button(sec_frame, text="2. Xin mã bảo mật Seed (0x27)", bg="#E3F2FD", command=self.req_seed).grid(row=0, column=1, padx=5, pady=5)
        tk.Button(sec_frame, text="3. Gửi Key mở khóa ECU", bg="#C8E6C9", font=("Arial", 9, "bold"), command=self.send_key).grid(row=0, column=2, padx=5, pady=5)

        # --- KHU VỰC 2: ĐIỀU KHIỂN HỆ THỐNG VÀ LỆNH KHẨN CẤP (MỚI BỔ SUNG) ---
        ctrl_frame = tk.LabelFrame(self, text=" 2. Điều khiển Vận hành & Hệ thống khẩn cấp ", padx=5, pady=5)
        ctrl_frame.pack(fill="x", pady=5)
        
        tk.Button(ctrl_frame, text="▶ KÍCH HOẠT XE CHẠY", bg="#4CAF50", fg="white", font=("Arial", 9, "bold"), width=20, command=self.start_car).grid(row=0, column=0, padx=5, pady=5)
        tk.Button(ctrl_frame, text="🛑 PHANH KHẨN CẤP", bg="#F44336", fg="white", font=("Arial", 9, "bold"), width=18, command=self.stop_car).grid(row=0, column=1, padx=5, pady=5)
        tk.Button(ctrl_frame, text="Kiểm Tra Trạng Thái", bg="#FFF9C4", command=self.read_run_status).grid(row=0, column=2, padx=5, pady=5)
        tk.Button(ctrl_frame, text="🔄 Reset ECU (0x11)", bg="#FFCCBC", fg="#D84315", command=self.reset_ecu).grid(row=0, column=3, padx=5, pady=5)

        # --- KHU VỰC 3: CÂN CHỈNH & ĐỌC/GHI THAM SỐ PID BÁM LÀN ---
        pid_frame = tk.LabelFrame(self, text=" 3. Cân chỉnh hệ số PID bám làn (DID 0x0200) ", padx=5, pady=5)
        pid_frame.pack(fill="x", pady=5)
        
        tk.Label(pid_frame, text="Kp:").grid(row=0, column=0, padx=2)
        self.ent_kp = tk.Entry(pid_frame, width=7); self.ent_kp.insert(0, "0.28"); self.ent_kp.grid(row=0, column=1, padx=4)
        
        tk.Label(pid_frame, text="Ki:").grid(row=0, column=2, padx=2)
        self.ent_ki = tk.Entry(pid_frame, width=7); self.ent_ki.insert(0, "0.001"); self.ent_ki.grid(row=0, column=3, padx=4)
        
        tk.Label(pid_frame, text="Kd:").grid(row=0, column=4, padx=2)
        self.ent_kd = tk.Entry(pid_frame, width=7); self.ent_kd.insert(0, "0.65"); self.ent_kd.grid(row=0, column=5, padx=4)
        
        tk.Button(pid_frame, text="📥 Đọc PID từ Xe (0x22)", bg="#E0F7FA", command=self.read_pid_param).grid(row=0, column=6, padx=6, pady=5)
        tk.Button(pid_frame, text="💾 Nạp PID xuống xe (0x2E)", bg="#FFB74D", font=("Arial", 9, "bold"), command=self.write_pid_param).grid(row=0, column=7, padx=6, pady=5)

    # --- CÁC HÀM XỬ LÝ CHO KHU VỰC 1 ---
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
                messagebox.showinfo("UDS Security", f"Nhận mã Seed thành công từ xe: {hex(self.saved_seed)}")
            else:
                messagebox.showerror("Lỗi", f"Không lấy được Seed. Trạng thái phản hồi: {parsed.get('status')}")
        else:
            messagebox.showerror("Lỗi kết nối", "Xe không phản hồi gói tin xin mã Seed!")

    def send_key(self):
        if self.saved_seed is None:
            messagebox.showwarning("Cảnh báo", "Vui lòng thực hiện bước xin mã Seed (0x27) trước!")
            return
        self.bt.send_uds_packet(enc.encode_send_key(self.saved_seed))
        res = self.bt.receive_uds_packet()
        if res:
            parsed = dec.decode_uds_response(res)
            if parsed.get("service") == "SECURITY_UNLOCKED":
                messagebox.showinfo("Thành công", "ECU Đã được mở khóa bảo mật hoàn toàn!")
            else:
                messagebox.showerror("Lỗi", "Mã thiết bị phát lên không khớp khóa lõi STM32!")
        else:
            messagebox.showerror("Lỗi kết nối", "Không nhận được phản hồi mở khóa từ xe!")

    # --- CÁC HÀM XỬ LÝ CHO KHU VỰC 2 (MỚI BỔ SUNG) ---
    def start_car(self):
        """Phát lệnh cho phép xe chạy thông qua ghi DID 0x0100 với giá trị 1"""
        self.bt.send_uds_packet(enc.encode_write_run_flag(True))
        res = self.bt.receive_uds_packet()
        if res:
            parsed = dec.decode_uds_response(res)
            if parsed.get("service") == "WRITE_SUCCESS":
                messagebox.showinfo("Vận hành", "Lệnh gửi THÀNH CÔNG! Đã kích hoạt cho phép xe chạy.")
            else:
                messagebox.showerror("Bị chặn lỗi", f"Không thể bật xe chạy. Mã NRC: {parsed.get('nrc')}\n(Nhắc nhở: Cậu đã mở khóa bảo mật bước 1-2-3 chưa?)")
        else:
            messagebox.showerror("Lỗi", "Xe không phản hồi lệnh kích hoạt động cơ!")

    def stop_car(self):
        """Phát lệnh phanh xe khẩn cấp thông qua ghi DID 0x0100 với giá trị 0"""
        self.bt.send_uds_packet(enc.encode_write_run_flag(False))
        res = self.bt.receive_uds_packet()
        if res:
            parsed = dec.decode_uds_response(res)
            if parsed.get("service") == "WRITE_SUCCESS":
                messagebox.showwarning("Khẩn cấp", "Đã phát lệnh PHANH DỪNG XE khẩn cấp thành công!")
            else:
                messagebox.showerror("Lỗi", f"Lệnh phanh bị từ chối. Mã NRC: {parsed.get('nrc')}")
        else:
            messagebox.showerror("Lỗi", "Xe không phản hồi lệnh phanh!")

    def read_run_status(self):
        """Đọc trạng thái xe hiện tại đang chạy hay đang phanh dừng bằng SID 0x22 - DID 0x0100"""
        self.bt.send_uds_packet(enc.encode_read_did(0x0100))
        res = self.bt.receive_uds_packet()
        if res:
            parsed = dec.decode_uds_response(res)
            if parsed.get("service") == "READ_RUN_FLAG":
                status = "ĐANG CHẠY (1)" if parsed.get("value") == 1 else "ĐANG PHANH DỪNG (0)"
                messagebox.showinfo("Đọc DID 0x0100", f"Trạng thái vận hành hiện tại từ ECU:\n▶ {status}")
            else:
                messagebox.showerror("Lỗi", "Không thể giải mã trạng thái vận hành.")
        else:
            messagebox.showerror("Lỗi", "Không nhận được phản hồi dữ liệu DID 0x0100!")

    def reset_ecu(self):
        """Phát lệnh ép cụm điều khiển trung tâm tái khởi động khẩn cấp (ECU Reset - 0x11)"""
        if messagebox.askyesno("Xác nhận", "Bạn có chắc chắn muốn ép mạch điều khiển xe khởi động lại từ xa không?"):
            self.bt.send_uds_packet(enc.encode_ecu_reset())
            res = self.bt.receive_uds_packet()
            if res:
                parsed = dec.decode_uds_response(res)
                if parsed.get("service") == "ECU_RESET_SUCCESS":
                    messagebox.showinfo("Thành công", "Mạch điều khiển trung tâm STM32 đã được Reset thành công!")
                else:
                    messagebox.showerror("Thất bại", f"ECU từ chối lệnh Reset. Mã phản hồi: {parsed.get('status')}")
            else:
                messagebox.showerror("Lỗi kết nối", "Không bắt được phản hồi xác nhận Reset từ phần cứng!")

    # --- CÁC HÀM XỬ LÝ CHO KHU VỰC 3 ---
    def read_pid_param(self):
        """Đọc ngược tham số Kp, Ki, Kd hiện tại đang lưu trong ô nhớ Flash bằng SID 0x22 - DID 0x0200"""
        self.bt.send_uds_packet(enc.encode_read_did(0x0200))
        res = self.bt.receive_uds_packet()
        if res:
            parsed = dec.decode_uds_response(res)
            if parsed.get("service") == "READ_PID":
                kp, ki, kd = parsed["kp"], parsed["ki"], parsed["kd"]
                
                # Xóa dữ liệu cũ và đồng bộ hiển thị thông số mới đọc được lên các ô nhập liệu UI
                self.ent_kp.delete(0, tk.END); self.ent_kp.insert(0, f"{kp:.4f}")
                self.ent_ki.delete(0, tk.END); self.ent_ki.insert(0, f"{ki:.5f}")
                self.ent_kd.delete(0, tk.END); self.ent_kd.insert(0, f"{kd:.4f}")
                messagebox.showinfo("Đọc DID 0x0200", f"Đồng bộ thành công hệ số PID từ bộ nhớ Flash:\n• Kp: {kp}\n• Ki: {ki}\n• Kd: {kd}")
            else:
                messagebox.showerror("Lỗi", "ECU từ chối quyền đọc hoặc sai cấu trúc gói tin.")
        else:
            messagebox.showerror("Lỗi", "Không phản hồi dữ liệu cấu hình PID!")

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