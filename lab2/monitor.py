import ctypes
from ctypes import wintypes
import sys
import time

PORT = r'\\.\COM5'
BAUD = 115200

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

# Mở cổng COM trực tiếp qua Win32 API (bỏ qua lỗi driver CH340 v3.9)
handle = kernel32.CreateFileW(
    PORT,
    0x80000000, # GENERIC_READ
    0,
    None,
    3, # OPEN_EXISTING
    0x80, # FILE_ATTRIBUTE_NORMAL
    None
)

if handle == -1:
    err = ctypes.get_last_error()
    print(f"[LỖI] Không thể mở cổng {PORT} (Mã lỗi: {err})")
    print("Vui lòng kiểm tra lại cáp cắm hoặc đóng các phần mềm đang giữ cổng.")
    sys.exit(1)

class COMMTIMEOUTS(ctypes.Structure):
    _fields_ = [
        ('ReadIntervalTimeout', wintypes.DWORD),
        ('ReadTotalTimeoutMultiplier', wintypes.DWORD),
        ('ReadTotalTimeoutConstant', wintypes.DWORD),
        ('WriteTotalTimeoutMultiplier', wintypes.DWORD),
        ('WriteTotalTimeoutConstant', wintypes.DWORD)
    ]

# Cấu hình timeout đọc 100ms
t = COMMTIMEOUTS(50, 0, 100, 0, 0)
kernel32.SetCommTimeouts(handle, ctypes.byref(t))

print("==========================================================")
print(f"  BKIT ARM4 - SERIAL MONITOR ĐANG CHẠY TRÊN {PORT} ({BAUD})")
print("  (Nhấn Ctrl + C để dừng)")
print("==========================================================")

buf = ctypes.create_string_buffer(256)
bytes_read = wintypes.DWORD()

try:
    while True:
        if kernel32.ReadFile(handle, buf, 255, ctypes.byref(bytes_read), None):
            if bytes_read.value > 0:
                text = buf.raw[:bytes_read.value].decode('latin1', errors='replace')
                sys.stdout.write(text)
                sys.stdout.flush()
        else:
            time.sleep(0.01)
except KeyboardInterrupt:
    print("\n[Đã dừng Serial Monitor]")
finally:
    kernel32.CloseHandle(handle)
