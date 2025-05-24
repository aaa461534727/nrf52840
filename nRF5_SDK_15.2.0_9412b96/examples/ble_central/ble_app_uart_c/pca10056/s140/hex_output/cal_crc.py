import os

def crc32_compute(data, initial=0xFFFFFFFF):
    """计算二进制数据的CRC32校验值"""
    crc = ~initial & 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320  # 多项式反转处理
            else:
                crc >>= 1
            crc &= 0xFFFFFFFF
    return ~crc & 0xFFFFFFFF

def calculate_file_crc(filename):
    """计算二进制文件的CRC32"""
    try:
        with open(filename, 'rb') as f:
            crc = 0xFFFFFFFF
            while True:
                chunk = f.read(8192)
                if not chunk:
                    break
                crc = crc32_compute(chunk, crc)
            return crc
    except Exception as e:
        print(f"文件读取错误: {e}")
        return None

def append_crc_with_auto_name(src_file):
    """生成带CRC校验值的新文件，自动命名格式：原文件名_crc校验值.bin"""
    try:
        # 获取原始CRC值
        original_crc = calculate_file_crc(src_file)
        if original_crc is None:
            return None

        # 生成新文件名（格式：原文件名_crc校验值.bin）
        base_name, ext = os.path.splitext(src_file)
        crc_hex = f"{original_crc:08x}".upper()  # 小写可改为 lower()
        dest_file = f"{base_name}_crc{crc_hex}{ext}"

        # 将CRC转换为4字节小端序
        crc_bytes = original_crc.to_bytes(4, byteorder='little')

        # 分块读写文件
        with open(src_file, 'rb') as f_in, open(dest_file, 'wb') as f_out:
            while True:
                chunk = f_in.read(8192)
                if not chunk:
                    break
                f_out.write(chunk)
            f_out.write(crc_bytes)

        return dest_file, original_crc
    except Exception as e:
        print(f"文件操作失败：{e}")
        return None

if __name__ == "__main__":
    original_file = "app.bin"
    
    # 生成新文件并获取信息
    result = append_crc_with_auto_name(original_file)
    
    if result:
        new_file, original_crc = result
        print(f"已生成带CRC校验的文件：{new_file}")
        print(f"文件 {original_file} 的CRC32校验值: 0x{original_crc:08x}")
        
        # 与标准库对比验证
        import zlib
        with open(original_file, 'rb') as f:
            std_crc = zlib.crc32(f.read()) & 0xFFFFFFFF
        print(f"标准库校验值对比: 0x{std_crc:08x} ({original_crc == std_crc})")