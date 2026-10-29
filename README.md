# ⚡ HỆ THỐNG NHÚNG (EMBEDDED SYSTEMS) - ZEPHYR RTOS
### Lập trình Đa luồng, Đồng bộ IPC & Mạng CAN Bus trên Kit BKIT ARM4 (STM32F407)

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (CO3037 / CO3038)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nền tảng phần cứng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành thời gian thực:** Zephyr RTOS v4.4.0 (Công cụ quản lý: `west`)

---

## 📖 Mục lục
- [1. Tổng quan dự án](#1-tổng-quan-dự-án)
- [2. Cấu trúc phân nhánh Git (Branching Strategy)](#2-cấu-trúc-phân-nhánh-git-branching-strategy)
- [3. Tóm tắt nội dung các bài thí nghiệm](#3-tóm-tắt-nội-dung-các-bài-thí-nghiệm)
  - [Lab 1: Đa luồng & Lập lịch thời gian thực](#-lab-1-đa-luồng--lập-lịch-thời-gian-thực)
  - [Lab 2: Đồng bộ tiến trình & Truyền thông IPC](#-lab-2-đồng-bộ-tiến-trình--truyền-thông-ipc)
  - [Lab 3: Mạng CAN Bus 2.0A/B & Hệ phân tán](#-lab-3-mạng-can-bus-20ab--hệ-phân-tán)
  - [Assignment: Thiết kế hệ thống nhúng thời gian thực](#-assignment-thiết-kế-hệ-thống-nhúng-thời-gian-thực)
- [4. Đặc tả phần cứng Kit BKIT ARM4](#4-đặc-tả-phần-cứng-kit-bkit-arm4)
- [5. Hướng dẫn thiết lập môi trường & Biên dịch](#5-hướng-dẫn-thiết-lập-môi-trường--biên-dịch)
- [6. Hướng dẫn thao tác với Git](#6-hướng-dẫn-thao-tác-với-git)
- [7. Tài liệu tham khảo](#7-tài-liệu-tham-khảo)

---

## 🎯 1. Tổng quan dự án

Repository này lưu trữ toàn bộ mã nguồn, cấu hình phần cứng (Devicetree, Kconfig) và báo cáo thực nghiệm cho môn học **Hệ thống nhúng** tại Trường ĐH Bách Khoa - ĐHQG-HCM. Dự án được triển khai trên nền tảng **Zephyr RTOS** (hệ điều hành thời gian thực mã nguồn mở hàng đầu do Linux Foundation bảo trợ), tiếp cận theo phong cách lập trình công nghiệp hiện đại:

- **Mô tả phần cứng chuẩn hóa (Hardware Abstraction):** Sử dụng **Devicetree Overlay** (`app.overlay`) tách biệt hoàn toàn giữa cấu hình chân ngoại vi và mã nguồn ứng dụng C.
- **Quản lý tính năng mô-đun (Configurability):** Tùy biến kernel linh hoạt qua hệ thống **Kconfig** (`prj.conf`).
- **Đa nhiệm tiền định (Deterministic Multitasking):** Khai thác bộ lập lịch ưu tiên (*Priority-based Preemptive & Cooperative Scheduler*), quản lý ngăn xếp luồng độc lập và cơ chế bảo vệ chống tràn stack phần cứng.
- **Đồng bộ hóa & Truyền thông IPC:** Ứng dụng Semaphore, Mutex và Message Queue trong xử lý ngắt trễ (*Deferred Interrupt Handling*) và chia sẻ dữ liệu liên luồng an toàn.
- **Mạng nhúng phân tán (Automotive/Industrial Bus):** Thiết kế mạng truyền thông vi điều khiển **CAN Bus 2.0A/B** với bộ lọc phần cứng (*Hardware Acceptance Filters*) và giao thức ứng dụng đồng bộ giữa nhiều trạm đo.

---

## 🌿 2. Cấu trúc phân nhánh Git (Branching Strategy)

Mỗi giai đoạn học tập và bài thực hành được quản lý trên một nhánh Git độc lập, đồng bộ đầy đủ mã nguồn Zephyr và file báo cáo kỹ thuật:

| Nhánh Git | Nội dung kỹ thuật chuyên sâu | Nền tảng & Giao thức | Báo cáo hoàn chỉnh |
| :--- | :--- | :--- | :---: |
| **`main`** | Tổng quan hệ sinh thái, ma trận bài học, kiến trúc RTOS & đặc tả Kit BKIT ARM4 | Zephyr RTOS / Docs | — |
| [**`lab1_multithreading`**](https://github.com/vophamk23/EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/tree/lab1_multithreading) | Đa luồng, Cooperative vs Preemptive, Quản lý Stack, Truyền cấu hình tham số luồng | Zephyr Kernel / Threads | PDF & LaTeX |
| [**`lab2_synchronization`**](https://github.com/vophamk23/EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/tree/lab2_synchronization) | Binary/Counting Semaphore, Mutex thừa kế ưu tiên, Message Queue, Ngắt trễ (Deferred ISR) | Zephyr IPC / Synchronization | PDF & LaTeX |
| [**`lab3_canbus`**](https://github.com/vophamk23/EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/tree/lab3_canbus) | Giao thức CAN 2.0A/B, Bộ điều khiển bxCAN, Hardware Filter, Heartbeat, Đồng bộ đa Node | CAN Bus / Distributed System | PDF & LaTeX |
| [**`assignment`**](https://github.com/vophamk23/EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/tree/assignment) | Thiết kế hệ thống nhúng thời gian thực hoàn chỉnh (Kiến trúc 4 tầng, FSM, Safety Watchdog) | Full-stack Embedded RTOS | Báo cáo BTL cuối kỳ |

---

## 📚 3. Tóm tắt nội dung các bài thí nghiệm

### 🧵 Lab 1: Đa luồng & Lập lịch thời gian thực
- **Trọng tâm:** Làm quen với kiến trúc đa luồng của Zephyr RTOS và cơ chế định thời phi chặn (`k_msleep`).
- **Nội dung thực hiện:**
  - Khởi tạo luồng tĩnh với macro `K_THREAD_DEFINE` và luồng động bằng `k_thread_create`.
  - Phân tích cơ chế phân cấp ưu tiên: **Cooperative threads** (độ ưu tiên âm: `-15` đến `-1`, không thể bị cướp quyền trừ khi chủ động nhường CPU) vs **Preemptive threads** (độ ưu tiên không âm: `0` đến `14`).
  - Thiết kế cấu trúc truyền tham số cho luồng (`struct blink_cfg`) để tái sử dụng một hàm luồng điều khiển đồng thời nhiều đèn LED với chu kỳ khác nhau.
  - Xây dựng máy trạng thái hữu hạn (FSM) mô phỏng hệ thống đèn giao thông độc lập.

---

### 🔄 Lab 2: Đồng bộ tiến trình & Truyền thông IPC
- **Trọng tâm:** Xử lý các sự cố kinh điển trong hệ đa nhiệm (*Race Condition*, *Data Corruption*, *Priority Inversion*).
- **Nội dung thực hiện:**
  - **Deferred Interrupt Processing:** Tách biệt ngắt phần cứng (GPIO ISR) và luồng xử lý nặng thông qua Binary Semaphore (`k_sem_give` từ ISR và `k_sem_take` từ Thread).
  - **Mutex (`k_mutex`):** Cơ chế loại trừ tương hỗ bảo vệ vùng nhớ tài nguyên dùng chung, tích hợp tính năng tự động thừa kế độ ưu tiên (*Priority Inheritance*).
  - **Message Queue (`k_msgq`):** Hàng đợi vòng truyền nhận các gói tin có cấu trúc an toàn theo mô hình Producer - Consumer.
  - Tuân thủ quy chuẩn xử lý ngắt: Không chặn (non-blocking) trong ISR với timeout bắt buộc `K_NO_WAIT`.

---

### 🚗 Lab 3: Mạng CAN Bus 2.0A/B & Hệ phân tán
- **Trọng tâm:** Lập trình mạng truyền thông công nghiệp CAN Bus (ISO 11898) trên bộ điều khiển STM32 bxCAN.
- **Nội dung thực hiện:**
  - Cấu hình Bit Timing chuẩn đạt tốc độ truyền thông **250 kbps** hoặc **500 kbps**.
  - Kiểm thử truyền nhận nội bộ với chế độ **Loopback Mode** và truyền nhận trên bus vật lý thực tế giữa 2 Kit Master - Slave (**Normal Mode**).
  - Thiết lập bộ lọc phần cứng (*Hardware Acceptance Mask & Filter*) bằng API `can_add_rx_filter()` giúp CPU vi điều khiển không bị đánh thức bởi các gói tin không liên quan.
  - Xây dựng hệ thống giám sát phân tán: Phát khung tín hiệu sống còn (*Heartbeat*) và đồng bộ tín hiệu giao thông liên trạm.

---

### 🚀 Assignment: Thiết kế hệ thống nhúng thời gian thực
- **Trọng tâm:** Đồ án tổng hợp thiết kế hệ sinh thái nhúng chuẩn công nghiệp theo kiến trúc 4 tầng:
  1. **Tầng Ứng dụng & FSM:** Điều khiển trạng thái hệ thống, phân tích tập lệnh HMI từ UART.
  2. **Tầng IPC & Dịch vụ:** Điều phối dữ liệu qua hàng đợi Message Queue và bảo vệ bộ nhớ chia sẻ bằng Mutex.
  3. **Tầng Mạng & Giao tiếp:** Giao thức ứng dụng trên nền CAN Bus kết nối cụm điều khiển và trạm cảm biến.
  4. **Tầng Trình điều khiển Phần cứng (Zephyr Drivers):** Tương tác với phần cứng thông qua API chuẩn của Zephyr.
  - Tích hợp cơ chế tự phục hồi sự cố (*Fail-Safe Supervisor*) và giám sát phần cứng bằng phần mềm.

---

## 🔌 4. Đặc tả phần cứng Kit BKIT ARM4

Bo mạch thí nghiệm **BKIT ARM4** do Khoa KH&KT Máy tính - ĐH Bách Khoa TP.HCM phát triển phục vụ đào tạo kỹ sư nhúng chuyên sâu:

### 4.1. Thông số kỹ thuật vi điều khiển STM32F407ZGT6
- **Lõi xử lý:** ARM 32-bit Cortex-M4 tích hợp đơn vị tính toán dấu phẩy động phần cứng (FPU) và tập lệnh DSP.
- **Xung nhịp tối đa:** **168 MHz** (sử dụng thạch anh ngoài HSE 8 MHz).
- **Bộ nhớ Flash:** **1024 KB** (1 MB).
- **Bộ nhớ SRAM:** **192 KB** (bao gồm 128 KB System SRAM + 64 KB Core Coupled Memory - CCM RAM tốc độ cao).

### 4.2. Bảng phân bố chân ngoại vi chuẩn (Pinout Mapping)
Toàn bộ chân ngoại vi được ánh xạ đồng bộ qua file Devicetree overlay (`app.overlay`):

| Tên ngoại vi | Ký hiệu trên Board | Chân GPIO STM32F407 | Alias Devicetree | Chế độ hoạt động / Logic |
| :--- | :--- | :--- | :--- | :--- |
| **LED Hệ thống** | LED DEBUG (LED3) | `PE4` | `led0` | Output Active-High (3.3V) |
| **Ngõ ra điều khiển 0** | Output Y0 | `PE5` | `led1` | Output Active-High |
| **Ngõ ra điều khiển 1** | Output Y1 | `PE6` | `led2` | Output Active-High |
| **Rơ-le công suất** | RELAY IN1 | `PE2` | `relay0` | Output điều khiển cuộn hút rơ-le |
| **Nút nhấn ngoại vi** | KEY / BTN | `PB12` | `sw0` | Input Active-Low (Kèm ngắt sườn xuống) |
| **CAN1 Receive** | CAN1_RX | `PB8` | `can1` | Alternate Function 9 (AF9) |
| **CAN1 Transmit** | CAN1_TX | `PB9` | `can1` | Alternate Function 9 (AF9) |
| **Cổng nạp & Gỡ lỗi** | SWD Header | `PA13` / `PA14` | `swd` | Kết nối mạch nạp ST-Link V2 |
| **Console UART** | MAX232 DB9 / Header | `PA9` / `PA10` | `usart1` | 115200 bps, 8-N-1 (Serial Log) |

---

## 💻 5. Hướng dẫn thiết lập môi trường & Biên dịch

Hệ điều hành khuyến nghị: **Ubuntu 22.04 LTS / 24.04 LTS** hoặc **Windows 11 (qua WSL2)**.

### 5.1. Khởi tạo Workspace Zephyr RTOS v4.4.0
```bash
# 1. Tạo môi trường ảo Python
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate    # Trên Windows: .venv\Scripts\activate

# 2. Cài đặt công cụ quản trị west
pip install west

# 3. Khởi tạo mã nguồn Zephyr
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0 ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```

### 5.2. Biên dịch và nạp firmware cho Kit BKIT ARM4
Dự án sử dụng target bo mạch `stm32f4_disco` kết hợp với file `app.overlay` tùy biến riêng cho BKIT ARM4:

```bash
# Di chuyển vào thư mục của ứng dụng cần nạp (Ví dụ: lab1_ex1_asym_blinky)
cd path/to/project_dir

# 1. Biên dịch ứng dụng
west build -p always -b stm32f4_disco .

# 2. Nạp chương trình vào vi điều khiển qua mạch nạp ST-Link
west flash

# 3. Dọn dẹp bộ nhớ đệm và build sạch lại từ đầu (khi sửa Devicetree/Kconfig)
west build -p pristine -b stm32f4_disco .
```

---

## 🚀 6. Hướng dẫn thao tác với Git

### 6.1. Clone toàn bộ mã nguồn
```bash
# Clone repository chính thức về máy
git clone https://github.com/vophamk23/EMBEDDED-SYSTEMS_Kit-BKIT-ARM4.git
cd EMBEDDED-SYSTEMS_Kit-BKIT-ARM4
```

### 6.2. Chuyển đổi giữa các nhánh bài thực hành
```bash
# Kiểm tra danh sách các nhánh
git branch -a

# Làm việc với Lab 1: Đa luồng & Lập lịch
git checkout lab1_multithreading

# Làm việc với Lab 2: Đồng bộ tiến trình & IPC
git checkout lab2_synchronization

# Làm việc với Lab 3: Mạng CAN Bus & Hệ phân tán
git checkout lab3_canbus

# Làm việc với Bài tập lớn cuối kỳ
git checkout assignment
```

---

## 📖 7. Tài liệu tham khảo

1. **Zephyr Project:** [Zephyr RTOS v4.4.0 Documentation](https://docs.zephyrproject.org/latest/)
2. **STMicroelectronics:** [STM32F407ZGT6 Reference Manual (RM0090)](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
3. **ARM Architecture:** [Cortex-M4 Generic User Guide](https://developer.arm.com/documentation/dui0553/latest/)
4. **Bosch GmbH:** [CAN Specification 2.0 Part A & Part B](https://www.bosch-semiconductors.com/ip-modules/can-ip-modules/)
5. **Khoa KH&KT Máy tính - ĐH Bách Khoa TP.HCM:** *Giáo trình & Sổ tay Thực hành Hệ thống nhúng (CO3037 / CO3038)*.
