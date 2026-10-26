# BÀI TẬP LỚN CUỐI KỲ: THIẾT KẾ HỆ THỐNG NHÚNG THỜI GIAN THỰC

Khoa Khoa học và Kỹ thuật Máy tính - Trường Đại học Bách Khoa, ĐHQG-HCM  
Môn học: Hệ thống nhúng (CO3037 / CO3038)  
Nhánh Git: `assignment`  
Nền tảng: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
Hệ điều hành: Zephyr RTOS v4.4.0  

---

## 1. Tổng quan đề tài Bài tập lớn

Nhánh `assignment` dành riêng cho việc thiết kế kiến trúc, lập trình và nghiệm thu Đề tài Bài tập lớn môn Hệ thống nhúng. Mục tiêu là tích hợp toàn diện các khối kiến thức nền tảng đã học trong suốt khóa học thành một hệ sinh thái nhúng chuẩn công nghiệp, bao gồm:
- **Kiến trúc đa nhiệm (Multitasking):** Thiết kế hệ thống chạy đồng thời nhiều luồng với mức độ ưu tiên (Priority) và dung lượng ngăn xếp (Stack) được tính toán tối ưu.
- **Đồng bộ và Truyền thông nội bộ (Synchronization & IPC):** Ứng dụng triệt để Mutex, Semaphore và Message Queue để luân chuyển dữ liệu an toàn giữa ISR và các Thread, loại bỏ hoàn toàn Race Condition và Deadlock.
- **Mạng truyền thông phân tán (Distributed CAN Network):** Kết nối nhiều vi điều khiển qua mạng CAN Bus 2.0A/B, thiết kế giao thức truyền thông tầng ứng dụng (Application-layer CAN Protocol).
- **Xử lý ngoại vi thời gian thực:** Thu thập dữ liệu cảm biến, điều khiển cơ cấu chấp hành (Rơ-le, Động cơ, Đèn tín hiệu) và tương tác người dùng qua UART Console.
- **Cơ chế an toàn hệ thống (Fail-safe & Fault Tolerance):** Giám sát sống còn (Heartbeat/Watchdog), tự động chuyển về trạng thái an toàn khi mất kết nối mạng hoặc xảy ra sự cố quá nhiệt.

---

## 2. Kiến trúc hệ thống phân tầng (Layered Architecture)

Hệ thống được tổ chức theo mô hình 4 tầng chuẩn công nghiệp:

```
+--------------------------------------------------------------------------+
|                     APPLICATION & FSM LAYER                              |
|  - System Finite State Machine (FSM)      - HMI & Command Parser         |
|  - Traffic / Plant Automation Logic       - Alarm & Fail-safe Supervisor |
+--------------------------------------------------------------------------+
|                     IPC & SERVICES LAYER                                 |
|  - Message Queues (Sensor / Command Data) - Mutex-protected Shared Memory|
|  - Event Semaphores (Deferred ISRs)       - Application CAN Protocol     |
+--------------------------------------------------------------------------+
|                     ZEPHYR RTOS KERNEL LAYER                             |
|  - Preemptive / Cooperative Scheduler     - Kernel Timers & Workqueues   |
|  - Memory & Stack Canary Protection       - System Tick & Clocks         |
+--------------------------------------------------------------------------+
|                     HARDWARE ABSTRACTION & BSP                           |
|  - STM32F407ZGT6 Peripherals: GPIO, bxCAN, USART1, Hardware Timers       |
|  - Devicetree Hardware Bindings (app.overlay)                            |
+--------------------------------------------------------------------------+
```

---

## 3. Cấu trúc thư mục đề xuất (Project Directory Structure)

Mỗi nhóm phát triển bài tập lớn cần duy trì tổ chức mã nguồn khoa học theo mẫu sau:

```
assignment/
├── docs/                                  # Tài liệu thiết kế và đặc tả hệ thống
│   ├── system_architecture_spec.md        # Sơ đồ kiến trúc và đặc tả yêu cầu (SRS)
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
│       └── can_master_service.c           # Xử lý truyền nhận CAN
├── node_remote/                           # Ứng dụng Node vệ tinh (Sensor / Actuator)
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── app.overlay
│   └── src/
│       ├── main.c
│       ├── sensor_acquisition.c
│       └── actuator_driver.c
└── README.md                              # Tài liệu tổng quan của nhánh assignment
```

---

## 4. Bảng phân bổ Luồng và Độ ưu tiên (Priority & Stack Planning)

Để tránh lỗi Deadlock hoặc chiếm dụng CPU kéo dài, hệ thống được quy hoạch mức ưu tiên như sau:

| Tên Luồng (Thread Name) | Mục đích thực thi | Độ ưu tiên (Priority) | Stack Size | Cơ chế IPC sử dụng |
| :--- | :--- | :--- | :--- | :--- |
| `isr_button_cb` | Xử lý ngắt phần cứng nút bấm | N/A (Hardware ISR) | N/A | `k_msgq_put(..., K_NO_WAIT)` |
| `thread_can_rx` | Nhận và phân loại bản tin CAN | Priority 2 (Cao) | 1536 B | `can_add_rx_filter`, `k_msgq` |
| `thread_supervisor` | Watchdog, kiểm tra sống còn Heartbeat | Priority 4 | 1024 B | `k_sem_take(timeout)` |
| `thread_main_fsm` | Máy trạng thái điều khiển hệ thống | Priority 6 | 2048 B | `k_msgq_get`, `k_mutex` |
| `thread_sensors` | Đọc và lọc tín hiệu cảm biến định kỳ | Priority 7 | 1024 B | `k_timer`, `k_msgq` |
| `thread_ui_logger` | In thông tin console UART, debug | Priority 10 (Thấp) | 1536 B | `k_msgq_get`, `printk` |

---

## 5. Đặc tả Giao thức CAN Bus ứng dụng (CAN Protocol Specification)

Hệ thống sử dụng chuẩn CAN 2.0A với định danh chuẩn (Standard 11-bit ID):

| Message ID | Loại khung | Chu kỳ / Sự kiện | Byte 0 - 1 | Byte 2 - 3 | Byte 4 - 5 | Byte 6 - 7 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `0x010` | Emergency / Alarm | Sự kiện (Async) | Node ID & Error Code | Sub-system State | Reserved | Rolling Counter |
| `0x100` | Control Command | Chu kỳ 200 ms | Command ID | Target Actuator | Param 1 | Param 2 |
| `0x200` | Sensor Telemetry | Chu kỳ 500 ms | Temperature (0.1°C) | Humidity (%) | Pressure (hPa) | Status Flags |
| `0x700` | Node Heartbeat | Chu kỳ 1000 ms | Node ID | Uptime (s) | System Status | CRC / Checksum |

---

## 6. Hướng dẫn Thiết lập và Biên dịch

Mỗi node trong hệ thống bài tập lớn được biên dịch độc lập bằng công cụ `west`:

```bash
# Biên dịch Node Master
west build -p always -b stm32f4_disco node_master

# Nạp firmware cho Kit Master
west flash

# Biên dịch Node Remote
west build -p always -b stm32f4_disco node_remote

# Nạp firmware cho Kit Remote
west flash
```

---

## 7. Quy chuẩn Đánh giá và Tiêu chí Nghiệm thu

1. **Tính ổn định và Thời gian thực (Real-time Determinism):** Hệ thống không bị treo (Lock-up), không tràn bộ nhớ (Stack Overflow) và đáp ứng đúng giới hạn thời gian thực đặt ra.
2. **Khả năng chống chịu lỗi (Fault Recovery):** Khi ngắt dây CAN, một trong các kit bị mất nguồn hoặc cảm biến gặp sự cố, hệ thống phải tự động chuyển sang chế độ Fail-safe và phục hồi ngay khi kết nối trở lại.
3. **Chất lượng mã nguồn:** Tuân thủ coding convention của Zephyr RTOS, chú thích rõ ràng, không hard-code các chân GPIO mà phải sử dụng Devicetree (`app.overlay`).
4. **Tài liệu kỹ thuật:** Báo cáo đồ án đầy đủ sơ đồ khối, sơ đồ FSM, phân tích độ trễ và nhật ký log thực nghiệm trên hệ thống phần cứng thật.
