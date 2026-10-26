# FINAL CAPSTONE ASSIGNMENT: REAL-TIME EMBEDDED SYSTEM DESIGN

Khoa Khoa hoc va Ky thuat May tinh - Truong Dai hoc Bach Khoa, DHQG-HCM  
Mon hoc: He thong nhung (CO3037 / CO3038)  
Nhanh Git: `assignment`  
Nen tang: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
He dieu hanh: Zephyr RTOS v4.4.0  

---

## 1. Tong quan de tai Bai tap lon

Nhanh `assignment` danh rieng cho viec thiet ke, phat trien va nghiem thu De tai Bai tap lon mon He thong nhung. Muc tieu la tich hop toan dien cac kien thuc nen tang da hoc trong suot khoa hoc thanh mot he thong nhung cong nghiep hoan chinh, bao gom:
- **Kien truc da nhiem (Multitasking):** Thiet ke he thong chay dong thoi nhieu luong voi do uu tien (Priority) va bo nho Stack duoc tinh toan toi uu.
- **Dong bo va Truyen thong noi bo (Synchronization & IPC):** Ap dung triet de Mutex, Semaphore va Message Queue de giao tiep an toan giua ISR va cac Thread, loai bo hoan toan Race Condition va Deadlock.
- **Mang truyen thong phan tan (Distributed CAN Network):** Ket noi nhieu Node vi dieu khien qua mang CAN Bus 2.0A/B, thiet ke giao thuc truyen thong tang ung dung (Application-layer CAN Protocol).
- **Xu ly ngoai vi thoi gian thuc:** Thu thap du lieu cam bien, dieu khien co cau chap hanh (Relay, Dong co, LED tin hieu) va tuong tac nguoi dung qua UART Console.
- **Co che an toan he thong (Fail-safe & Fault Tolerance):** Giam sat song con (Heartbeat/Watchdog), phat hien su co mat mang hoac qua nhiet de dua he thong ve trang thai an toan.

---

## 2. Kien truc he thong phan tang (Layered Architecture)

He thong duoc to chuc theo mo hinh 4 tang chuan cong nghiep:

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

## 3. Cau truc thu muc de xuat (Project Directory Structure)

Moi nhom phat trien bai tap lon can duy tri to chuc ma nguon khoa hoc theo mau sau:

```
assignment/
├── docs/                                  # Tai lieu thiet ke va dac ta
│   ├── system_architecture_spec.md        # So do kien truc va dac ta yeu cau (SRS)
│   ├── can_protocol_specification.md      # Dinh nghia khung truyen CAN Bus
│   └── hardware_schematic_mapping.md      # Ban ve so do noi day ngoai vi
├── common/                                # Thu vien dung chung giua cac module/node
│   ├── can_protocol.h                     # Dinh nghia cac Message ID, DLC, Payload
│   ├── system_config.h                    # Hang so he thong, nguong canh bao, timeout
│   └── error_codes.h                      # Bang ma loi he thong thong nhat
├── node_master/                           # Ung dung Node dieu khien trung tam (neu he phan tan)
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── app.overlay
│   └── src/
│       ├── main.c                         # Khoi tao va luong quan ly FSM
│       ├── threads_supervisor.c           # Luong giam sat mang va an toan
│       └── can_master_service.c           # Xu ly truyen nhan CAN
├── node_remote/                           # Ung dung Node ve tinh (Sensor / Actuator)
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── app.overlay
│   └── src/
│       ├── main.c
│       ├── sensor_acquisition.c
│       └── actuator_driver.c
└── README.md                              # Tai lieu tong quan cua nhanh assignment
```

---

## 4. Bang phan bo Luong va Do uu tien (Priority & Stack Planning)

De tranh loi Deadlock hoac chiem dung CPU, he thong duoc quy hoach do uu tien nhu sau:

| Ten Luong (Thread Name) | Muc dich thuc thi | Do uu tien (Priority) | Stack Size | Co che IPC su dung |
| :--- | :--- | :--- | :--- | :--- |
| `isr_button_cb` | Xu ly ngat phan cung nut bam | N/A (Hardware ISR) | N/A | `k_msgq_put(..., K_NO_WAIT)` |
| `thread_can_rx` | Nhan va phan loai ban tin CAN | Priority 2 (Cao) | 1536 B | `can_add_rx_filter`, `k_msgq` |
| `thread_supervisor` | Watchdog, kiem tra song con Heartbeat | Priority 4 | 1024 B | `k_sem_take(timeout)` |
| `thread_main_fsm` | May trang thai dieu khien he thong | Priority 6 | 2048 B | `k_msgq_get`, `k_mutex` |
| `thread_sensors` | Doc va loc tin hieu cam bien dinh ky | Priority 7 | 1024 B | `k_timer`, `k_msgq` |
| `thread_ui_logger` | In thong tin console UART, debug | Priority 10 (Thap) | 1536 B | `k_msgq_get`, `printk` |

---

## 5. Dac ta Giao thuc CAN Bus ung dung (CAN Protocol Specification)

He thong su dung chuan CAN 2.0A voi Standard Identifier (11-bit ID):

| Message ID | Loai khung | Chu ky / Su kien | Byte 0 - 1 | Byte 2 - 3 | Byte 4 - 5 | Byte 6 - 7 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `0x010` | Emergency / Alarm | Su kien (Async) | Node ID & Error Code | Sub-system State | Reserved | Rolling Counter |
| `0x100` | Control Command | Chu ky 200 ms | Command ID | Target Actuator | Param 1 | Param 2 |
| `0x200` | Sensor Telemetry | Chu ky 500 ms | Temperature (0.1 C) | Humidity (%) | Pressure (hPa) | Status Flags |
| `0x700` | Node Heartbeat | Chu ky 1000 ms | Node ID | Uptime (s) | System Status | CRC / Checksum |

---

## 6. Huong dan Thiet lap va Bien dich

Moi node trong he thong bai tap lon duoc build doc lap bang `west`:

```bash
# Bien dich Node Master
west build -p always -b stm32f4_disco node_master

# Nap firmware cho Kit Master
west flash

# Bien dich Node Remote
west build -p always -b stm32f4_disco node_remote

# Nap firmware cho Kit Remote
west flash
```

---

## 7. Quy chuan Danh gia va Tieu chi Nghiem thu

1. **Tinh on dinh va Thoi gian thuc (Real-time Determinism):** He thong khong bi treo (Lock-up), khong tran bo nho (Stack Overflow) va dap ung dung thoi gian thuc dat ra.
2. **Kha nang chong chiu loi (Fault Recovery):** Khi ngat day CAN, mot trong cac kit bi mat nguon hoac cam bien gap su co, he thong phai tu dong chuyen sang che do Fail-safe va khoi phuc ngay khi ket noi tro lai.
3. **Chat luong ma nguon:** Tuan thu coding convention cua Zephyr RTOS, chu thich ro rang, khong hard-code cac chan GPIO ma phai su dung Devicetree (`app.overlay`).
4. **Tai lieu ky thuat:** Ban bao cao do an day du so do khoi, so do FSM, phan tich do tre va nhat ky log thuc nghiem tren he thong phan cung that.
