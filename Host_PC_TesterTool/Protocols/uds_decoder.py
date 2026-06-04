import struct

def decode_uds_response(rx_bytes):
    """Giải mã gói dữ liệu UDS thô nhận được từ ECU xe"""
    if len(rx_bytes) < 2:
        return {"status": "ERROR", "msg": "Gói dữ liệu quá ngắn"}
    
    length = rx_bytes[0]
    sid_response = rx_bytes[1]
    
    # 1. TRƯỜNG HỢP PHẢN HỒI LỖI (Negative Response luôn có byte nhận diện là 0x7F)
    if sid_response == 0x7F:
        rejected_sid = rx_bytes[2] if len(rx_bytes) > 2 else 0x00
        nrc = rx_bytes[3] if len(rx_bytes) > 3 else 0x00
        return {"status": "NEGATIVE", "sid": hex(rejected_sid), "nrc": hex(nrc)}
    
    # 2. TRƯỜNG HỢP PHẢN HỒI THÀNH CÔNG (Positive Response = SID gốc + 0x40)
    if sid_response == 0x10 + 0x40:  # 0x50
        return {"status": "SUCCESS", "service": "SESSION", "session": rx_bytes[2]}
    
    elif sid_response == 0x27 + 0x40:  # 0x67
        sub_fn = rx_bytes[2]
        if sub_fn == 0x01:  # Nhận mã Seed thành công
            seed = struct.unpack('<I', rx_bytes[3:7])[0]
            return {"status": "SUCCESS", "service": "SEED", "value": seed}
        elif sub_fn == 0x02:  # Đã mở khóa an toàn thành công
            return {"status": "SUCCESS", "service": "SECURITY_UNLOCKED"}
            
    elif sid_response == 0x22 + 0x40:  # 0x62 (Đọc dữ liệu DID)
        did = (rx_bytes[2] << 8) | rx_bytes[3]
        data = rx_bytes[4:]
        
        if did == 0x0100:
            return {"status": "SUCCESS", "service": "READ_RUN_FLAG", "value": data[0]}
        elif did == 0x0200:
            kp, ki, kd = struct.unpack('<fff', data[0:12])
            return {"status": "SUCCESS", "service": "READ_PID", "kp": kp, "ki": ki, "kd": kd}
        elif did == 0x0300:
            enc_l, enc_r = struct.unpack('<ii', data[0:8])
            return {"status": "SUCCESS", "service": "READ_ENCODER", "left": enc_l, "right": enc_r}
        elif did == 0x0400:
            # Lưu ý đặc biệt: Riêng mã lỗi sai số làn đường dưới firmware đang băm Big-Endian ('>h')
            error = struct.unpack('>h', data[0:2])[0]
            return {"status": "SUCCESS", "service": "READ_LINE_ERROR", "error": error}
            
    elif sid_response == 0x2E + 0x40:  # 0x6E
        did = (rx_bytes[2] << 8) | rx_bytes[3]
        return {"status": "SUCCESS", "service": "WRITE_SUCCESS", "did": hex(did)}
        
    elif sid_response == 0x11 + 0x40:  # 0x51
        return {"status": "SUCCESS", "service": "ECU_RESET_SUCCESS"}
        
    return {"status": "UNKNOWN", "raw": rx_bytes.hex()}