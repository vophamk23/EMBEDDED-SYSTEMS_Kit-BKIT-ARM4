# 🚀 BÀI TẬP LỚN CUỐI KỲ: THIẾT KẾ HỆ THỐNG NHÚNG THỜI GIAN THỰC
### Kiến trúc 4 tầng, Đa nhiệm Zephyr RTOS, Đồng bộ IPC & Mạng CAN Bus Phân tán

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (CO3037 / CO3038)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `assignment` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Tổng quan đề tài Bài tập lớn](#1-tổng-quan-đề-tài-bài-tập-lớn)
- [2. Kiến trúc hệ thống phân tầng (Layered Architecture)](#2-kiến-trúc-hệ-thống-phân-tầng-layered-architecture)
- [3. Cấu trúc thư mục dự án](#3-cấu-trúc-thư-mục-dự-án)
- [4. Phân bổ Luồng và Độ ưu tiên (Priority & Stack Planning)](#4-phân-bổ-luồng-và-độ-ưu-tiên-priority--stack-planning)
- [5. Đặc tả Giao thức CAN Bus ứng dụng](#5-đặc-tả-giao-thức-can-bus-ứng-dụng)
- [6. Hướng dẫn thiết lập, Biên dịch & Nạp firmware](#6-hướng-dẫn-thiết-lập-biên-dịch--nạp-firmware)
- [7. Quy chuẩn đánh giá & Tiêu chí nghiệm thu](#7-quy-chuẩn-đánh-giá--tiêu-chí-nghiệm-thu)

---

## 🎯 1. Tổng quan đề tài Bài tập lớn

Nhánh `assignment` dành riêng cho việc thiết kế kiến trúc, hiện thực mã nguồn và nghiệm thu **Đề tài Bài tập lớn môn Hệ thống nhúng**. Mục tiêu là tích hợp toàn diện các khối kiến thức nền tảng đã học trong suốt khóa học thành một hệ sinh thái nhúng chuẩn công nghiệp:

- **Kiến trúc đa nhiệm thời gian thực (Real-time Multitasking):** Thiết kế hệ thống chạy đồng thời nhiều luồng với mức độ ưu tiên (*Priority*) và dung lượng ngăn xếp (*Stack Size*) được tính toán tối ưu, loại trừ triệt để lỗi tràn bộ nhớ.
- **Đồng bộ hóa & Truyền thông nội bộ (Synchronization & IPC):** Ứng dụng triệt để Mutex, Semaphore và Message Queue để luân chuyển dữ liệu an toàn giữa ngắt ngoại vi (ISR) và các luồng ứng dụng (*Threads*), ngăn ngừa tuyệt đối xung đột tài nguyên (*Race Condition*) và tắc nghẽn (*Deadlock*).
- **Mạng truyền thông vi điều khiển phân tán (Distributed CAN Network):** Kết nối nhiều Kit BKIT ARM4 qua mạng CAN Bus 2.0A/B, thiết kế giao thức truyền thông tầng ứng dụng (*Application-layer CAN Protocol*) với cơ chế phân xử ID không phá hủy.
- **Xử lý ngoại vi thời gian thực:** Thu thập số liệu cảm biến, điều khiển cơ cấu chấp hành (Rơ-le công suất, đèn tín hiệu) và giao tiếp người dùng qua cổng UART Console.
- **Cơ chế an toàn hệ thống (Fail-Safe & Fault Tolerance):** Giám sát sống còn (*Heartbeat / Watchdog*), tự động kích hoạt chế độ bảo vệ khi mất kết nối bus hoặc phát hiện sự cố quá nhiệt.

---

## 🏗️ 2. Kiến trúc hệ thống phân tầng (Layered Architecture)

Hệ thống được thiết kế theo mô hình 4 tầng chuẩn công nghiệp, tách biệt rõ ràng giữa logic ứng dụng và phần cứng vi điều khiển:

```text
+--------------------------------------------------------------------------+
|                     1. TẦNG ỨNG DỤNG & FSM (APPLICATION)                |
|  - System Finite State Machine (FSM)      - HMI & Command Parser         |
|  - Traffic / Automation Logic             - Alarm & Fail-Safe Supervisor |
+--------------------------------------------------------------------------+
|                     2. TẦNG IPC & DỊCH VỤ (SERVICES)                    |
|  - Message Queues (Sensor / Command Data) - Mutex-protected Shared Memory|
|  - Event Semaphores (Deferred ISRs)       - Application CAN Protocol     |
+--------------------------------------------------------------------------+
|                     3. TẦNG NHÂN ZEPHYR RTOS (KERNEL)                    |
|  - Preemptive / Cooperative Scheduler     - Kernel Timers & Workqueues   |
|  - Hardware Stack Canary Protection       - System Tick & Clocks         |
+--------------------------------------------------------------------------+
|                     4. TẦNG PHẦN CỨNG & BSP (HARDWARE)                   |
|  - STM32F407ZGT6: GPIO, bxCAN, USART1, Timers                            |
|  - Devicetree Hardware Bindings (app.overlay)                            |
+--------------------------------------------------------------------------+
```

---

## 📁 3. Cấu trúc thư mục dự án

```text
assignment/
├── docs/                                  # Tài liệu thiết kế và đặc tả hệ thống
│   ├── system_architecture_spec.md        # Sơ đồ kiến trúc & đặc tả yêu cầu SRS
│   ├── can_protocol_specification.md      # Định nghĩa khung truyền CAN Bus
│   └── hardware_schematic_mapping.md      # Bản vẽ sơ đồ nối dây ngoại vi
├── common/                                # Thư viện dùng chung giữa các node
│   ├── can_protocol.h                     # Định nghĩa Message ID, DLC, Payload
│   ├── system_config.h                    # Hằng số hệ thống, ngưỡng cảnh báo, timeout
│   └── error_codes.h                      # Bảng mã lỗi hệ thống thống nhất
├── node_master/                           # Ứng dụng Node điều khiển trung tâm
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── app.overlay
│   └── src/
│       ├── main.c                         # Khởi tạo và luồng quản lý FSM
│       ├── threads_supervisor.c           # Luồng giám sát mạng và an toàn
│       └── can_master_service.c           # Xử lý truyền nhận CAN Bus
├── node_remote/                           # Ứng dụng Node vệ tinh (Sensor / Actuator)
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── app.overlay
│   └── src/
│       ├── main.c
│       ├── sensor_acquisition.c
│       └── actuator_driver.c
└── README.md                              # Tài liệu hướng dẫn Bài tập lớn
```

---

## 📊 4. Phân bổ Luồng và Độ ưu tiên (Priority & Stack Planning)

Để ngăn ngừa lỗi Deadlock hoặc chiếm dụng CPU quá mức, hệ thống quy hoạch mức ưu tiên và kích thước ngăn xếp như sau:

| Tên Luồng (*Thread Name*) | Mục đích thực thi | Mức ưu tiên (*Priority*) | Dung lượng Stack | Cơ chế IPC sử dụng |
| :--- | :--- | :---: | :---: | :--- |
| **`isr_button_cb`** | Xử lý ngắt phần cứng nút bấm ngoại vi | N/A (Hardware ISR) | N/A | `k_msgq_put(..., K_NO_WAIT)` |
| **`thread_can_rx`** | Đón nhận và phân loại gói tin CAN | Priority 2 (Rất cao) | 1536 B | `can_add_rx_filter`, `k_msgq` |
| **`thread_supervisor`**| Giám sát sống còn Heartbeat / Watchdog | Priority 4 (Cao) | 1024 B | `k_sem_take(timeout)` |
| **`thread_main_fsm`** | Máy trạng thái điều khiển trung tâm | Priority 6 (Trung bình)| 2048 B | `k_msgq_get`, `k_mutex` |
| **`thread_sensors`**  | Lấy mẫu và tính toán lọc số cảm biến | Priority 7 (Trung bình)| 1024 B | `k_timer`, `k_msgq` |
| **`thread_ui_logger`**| Xuất nhật ký gỡ lỗi ra cổng UART | Priority 10 (Thấp) | 1536 B | `k_msgq_get`, `printk` |

---

## 🔌 5. Đặc tả Giao thức CAN Bus ứng dụng

Mạng truyền thông sử dụng chuẩn CAN 2.0A với định danh chuẩn (Standard 11-bit ID):

| Message ID | Loại khung | Chu kỳ / Sự kiện | Byte 0 - 1 | Byte 2 - 3 | Byte 4 - 5 | Byte 6 - 7 |
| :---: | :--- | :--- | :--- | :--- | :--- | :--- |
| **`0x010`** | Khẩn cấp (*Alarm*) | Sự kiện (*Async*) | Node ID & Error Code | Sub-system State | Reserved | Rolling Counter |
| **`0x100`** | Lệnh (*Command*) | Chu kỳ 200 ms | Command ID | Target Actuator | Param 1 | Param 2 |
| **`0x200`** | Đo đạc (*Telemetry*)| Chu kỳ 500 ms | Nhiệt độ ($0.1^\circ\text{C}$) | Độ ẩm (%RH) | Áp suất (hPa) | Status Flags |
| **`0x700`** | Sống còn (*Heartbeat*)| Chu kỳ 1000 ms | Node ID | Uptime (giây) | System Status | Checksum CRC |

---

## 💻 6. Hướng dẫn thiết lập, Biên dịch & Nạp firmware

Mỗi Node trong hệ thống được biên dịch độc lập thông qua công cụ `west`:

```bash
# 1. Biên dịch và nạp Node Master (Kit điều khiển trung tâm)
west build -p always -b stm32f4_disco node_master
west flash

# 2. Biên dịch và nạp Node Remote (Kit cảm biến / chấp hành ngoại vi)
west build -p always -b stm32f4_disco node_remote
west flash

# 3. Mở Serial Console giám sát trạng thái tương tác liên trạm
picocom -b 115200 /dev/ttyUSB0
```

---

## ⚙️ 7. Quy chuẩn đánh giá & Tiêu chí nghiệm thu

1. **Tính tiền định và Thời gian thực (Real-time Determinism):** Hệ thống không bị treo (*Lock-up*), không xảy ra tràn ngăn xếp (*Stack Overflow*) và đáp ứng chính xác các mốc thời gian (*Deadlines*).
2. **Khả năng tự phục hồi sự cố (Fault Recovery):** Khi ngắt kết nối dây CAN, mất nguồn đột ngột ở một Node hoặc cảm biến gặp lỗi, hệ thống phải tự động chuyển sang chế độ an toàn (*Fail-Safe*) và phục hồi bình thường ngay khi nối lại mạng.
3. **Chất lượng mã nguồn:** Tuân thủ chuẩn lập trình của Zephyr RTOS, không hard-code địa chỉ thanh ghi hay chân GPIO mà bắt buộc định nghĩa thông qua Devicetree (`app.overlay`).
4. **Hồ sơ tài liệu hoàn chỉnh:** Báo cáo kỹ thuật chi tiết gồm sơ đồ khối kiến trúc, sơ đồ chuyển trạng thái FSM, bảng phân tích độ trễ và hình ảnh thực nghiệm trên Kit BKIT ARM4.
