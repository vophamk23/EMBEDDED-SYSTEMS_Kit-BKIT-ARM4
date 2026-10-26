# HỆ THỐNG NHÚNG (EMBEDDED SYSTEMS) - ZEPHYR RTOS APPLICATIONS

Khoa Khoa học và Kỹ thuật Máy tính - Trường Đại học Bách Khoa, ĐHQG-HCM  
Môn học: Hệ thống nhúng (CO3037 / CO3038)  
Nền tảng phần cứng: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
Hệ điều hành thời gian thực: Zephyr RTOS v4.4.0  

---

## 1. Tổng quan dự án

Repository này chứa toàn bộ mã nguồn, cấu hình Devicetree, Kconfig và tài liệu hướng dẫn thực hành môn học Hệ thống nhúng. Dự án được thiết kế dựa trên hệ điều hành thời gian thực mã nguồn mở chuẩn công nghiệp Zephyr RTOS, tập trung vào các kỹ năng kỹ thuật cốt lõi:
- Lập trình hệ thống đa luồng (Multithreading) và lập lịch thời gian thực (Real-time Scheduling).
- Đồng bộ tiến trình và truyền thông liên tiến trình (Synchronization & IPC: Semaphores, Mutex, Message Queue).
- Mạng truyền thông vi điều khiển công nghiệp Controller Area Network (CAN Bus 2.0A/B) và hệ thống phân tán.
- Tích hợp hệ thống nhúng thời gian thực hoàn chỉnh trong Đồ án cuối kỳ (Capstone Assignment).

---

## 2. Cấu trúc các nhánh (Git Branching Strategy)

Hệ thống mã nguồn được tổ chức thành các nhánh độc lập phục vụ cho từng giai đoạn đào tạo và nghiên cứu:

| Tên nhánh | Nội dung chuyên biệt | Mục tiêu đào tạo |
| :--- | :--- | :--- |
| `main` | Nhánh tổng hợp chứa toàn bộ tài liệu tổng quan, ma trận bài học và đặc tả phần cứng. | Tham chiếu chung toàn khóa |
| `lab1_multithreading` | Cơ bản về Zephyr RTOS, quản lý đa luồng, định thời, mức độ ưu tiên thực thi. | Nền tảng RTOS & Đa nhiệm |
| `lab2_synchronization` | Đồng bộ tiến trình, Semaphore, Mutex chống xung đột tài nguyên, Message Queue, Deferred ISR. | Đồng bộ & Truyền thông IPC |
| `lab3_canbus` | Giao thức CAN 2.0A/B, bộ lọc phần cứng (Hardware Filters), khung RTR, mạng 2 node phân tán. | Mạng nhúng & Hệ phân tán |
| `assignment` | Khung kiến trúc đồ án cuối kỳ tích hợp đa luồng, IPC, mạng CAN Bus và ngoại vi. | Đồ án tổng hợp (Capstone) |

---

## 3. Đặc tả phần cứng Kit BKIT ARM4

Hệ thống sử dụng board phát triển BKIT ARM4 được thiết kế bởi ĐH Bách Khoa TP.HCM cho mục đích đào tạo kỹ sư nhúng:

### 3.1. Thông số kỹ thuật vi điều khiển
- Vi điều khiển: STM32F407ZGT6 (STMicroelectronics).
- Lõi xử lý: ARM 32-bit Cortex-M4 với FPU (Floating Point Unit) và tập lệnh DSP.
- Tần số hoạt động: 168 MHz (sử dụng thạch anh ngoài HSE 8 MHz).
- Bộ nhớ Flash: 1024 KB (1 MB).
- Bộ nhớ SRAM: 192 KB (128 KB SRAM1/SRAM2 + 64 KB CCM SRAM).

### 3.2. Bảng phân bố chân ngoại vi (Pinout Mapping)
Các chân ngoại vi trên kit được định nghĩa đồng nhất thông qua file Devicetree overlay (`app.overlay`):

| Chức năng | Tên trên Board | Chân GPIO STM32F407 | Chế độ / Mức logic |
| :--- | :--- | :--- | :--- |
| LED Hệ thống | LED DEBUG (LED3) | `PE4` | Output, Active High (`led0`) |
| Ngõ ra điều khiển 0 | Output Y0 | `PE5` | Output, Active High (`led1`) |
| Ngõ ra điều khiển 1 | Output Y1 | `PE6` | Output, Active High (`led2`) |
| Rơ-le công suất | RELAY IN1 | `PE2` | Output, Active High |
| Nút nhấn ngoại vi | KEY / BTN | `PB12` | Input, Active Low (Pull-up) |
| CAN1 Receive | CAN1_RX | `PB8` | Alternate Function 9 (AF9) |
| CAN1 Transmit | CAN1_TX | `PB9` | Alternate Function 9 (AF9) |
| Console Debug | USART1_TX / RX | `PA9` / `PA10` | 115200 bps, 8-N-1 (qua cổng MAX232 DB9) |

---

## 4. Yêu cầu môi trường và Cài đặt

### 4.1. Công cụ cần thiết
- Hệ điều hành: Ubuntu Linux 22.04 LTS / 24.04 LTS hoặc Windows 10/11 (WSL2 / PowerShell).
- Trình quản lý dự án Zephyr: `west` (phiên bản 1.2.0 trở lên).
- Bộ công cụ biên dịch: Zephyr SDK (v0.16.x trở lên) bao gồm trình biên dịch `arm-zephyr-eabi-gcc`.
- Trình nạp chương trình: OpenOCD / ST-Link CLI.
- Phần mềm giám sát UART: PuTTY, Tera Term, hoặc Serial Monitor trong VS Code (Baudrate: 115200).

### 4.2. Khởi tạo workspace Zephyr
Nếu chưa có sẵn workspace Zephyr trên máy tính, thực hiện theo quy trình chuẩn:

```bash
# Tạo môi trường ảo Python
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate    # Trên Windows: .venv\Scripts\activate

# Cài đặt công cụ west
pip install west

# Khởi tạo mã nguồn Zephyr v4.4.0
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0 ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```

---

## 5. Hướng dẫn sử dụng Repository

### 5.1. Clone mã nguồn
```bash
git clone https://github.com/vophamk23/EMBEDDED-SYSTEMS.git
cd EMBEDDED-SYSTEMS
```

### 5.2. Chuyển đổi qua lại giữa các nhánh
Để làm việc với từng bài thực hành, chuyển nhánh tương ứng bằng Git:

```bash
# Xem danh sách các nhánh trên máy chủ remote
git branch -a

# Chuyển sang Lab 1: Đa luồng
git checkout lab1_multithreading

# Chuyển sang Lab 2: Đồng bộ và IPC
git checkout lab2_synchronization

# Chuyển sang Lab 3: Mạng CAN Bus
git checkout lab3_canbus

# Chuyển sang Assignment: Đồ án cuối kỳ
git checkout assignment
```

### 5.3. Quy trình biên dịch và nạp chương trình (Build & Flash)
Mỗi thư mục bài tập (ví dụ: `lab1_ex1_asym_blinky`) là một ứng dụng Zephyr độc lập. Thực hiện lệnh sau từ thư mục gốc của nhánh:

```bash
# Biên dịch ứng dụng (sử dụng board stm32f4_disco được overlay bởi app.overlay)
west build -p always -b stm32f4_disco path/to/project_dir

# Nạp firmware vào vi điều khiển qua ST-Link
west flash

# Xóa bộ nhớ đệm và biên dịch lại hoàn toàn từ đầu
west build -p pristine -b stm32f4_disco path/to/project_dir
```

---

## 6. Danh mục tài liệu tham khảo

1. Tài liệu chính thức Zephyr RTOS (v4.4.0): https://docs.zephyrproject.org/latest/
2. STM32F407ZGT6 Reference Manual (RM0090): STMicroelectronics.
3. Cortex-M4 Generic User Guide: ARM Holdings.
4. Giáo trình Thực hành Hệ thống nhúng - Khoa KH&KT Máy tính, ĐH Bách Khoa TP.HCM.
