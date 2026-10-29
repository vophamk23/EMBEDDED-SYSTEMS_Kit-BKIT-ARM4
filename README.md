# 🔄 LAB 2: ĐỒNG BỘ TIẾN TRÌNH VÀ TRUYỀN THÔNG LIÊN TIẾN TRÌNH (IPC) TRONG ZEPHYR RTOS
### Binary & Counting Semaphore, Mutex Thừa Kế Ưu Tiên, Message Queue & Deferred ISR

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (CO3037 / CO3038)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `lab2_synchronization` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Mục tiêu thí nghiệm](#1-mục-tiêu-thí-nghiệm)
- [2. Cấu trúc thư mục dự án](#2-cấu-trúc-thư-mục-dự-án)
- [3. Phân bố chân phần cứng & Devicetree](#3-phân-bố-chân-phần-cứng--devicetree)
- [4. Danh mục các bài thực hành chi tiết](#4-danh-mục-các-bài-thực-hành-chi-tiết)
  - [4.1. Các bài Demo nền tảng (demos/)](#41-các-bài-demo-nền-tảng-demos)
  - [4.2. Các bài tập ứng dụng thực hành (lab2_ex*)](#42-các-bài-tập-ứng-dụng-thực-hành-lab2_ex)
    - [Bài 1: lab2_ex1_relay_binary_sem - Rơ-le bảo vệ quá nhiệt](#bài-1-lab2_ex1_relay_binary_sem---rơ-le-bảo-vệ-quá-nhiệt)
    - [Bài 2: lab2_ex2_timeout_monitor - Giám sát lỗi Timeout cảm biến](#bài-2-lab2_ex2_timeout_monitor---giám-sát-lỗi-timeout-cảm-biến)
    - [Bài 3: lab2_ex3_counting_sem_workers - Quản lý Thread Pool](#bài-3-lab2_ex3_counting_sem_workers---quản-lý-thread-pool)
    - [Bài 4: lab2_ex4_isr_message_queue - Giao tiếp ISR tới Thread](#bài-4-lab2_ex4_isr_message_queue---giao-tiếp-isr-tới-thread)
    - [Bài 5: lab2_ex5_pedestrian_traffic_light - Đèn giao thông ưu tiên](#bài-5-lab2_ex5_pedestrian_traffic_light---đèn-giao-thông-ưu-tiên)
- [5. Hướng dẫn biên dịch & Nạp chương trình](#5-hướng-dẫn-biên-dịch--nạp-chương-trình)
- [6. Quy tắc kỹ thuật & Lưu ý cốt lõi](#6-quy-tắc-kỹ-thuật--lưu-ý-cốt-lõi)

---

## 🎯 1. Mục tiêu thí nghiệm

1. **Làm chủ các cơ chế IPC (Inter-Process Communication) chuẩn công nghiệp:**
   - **Binary Semaphore (`k_sem`):** Cơ chế đồng bộ hóa 1-1 giữa luồng sự kiện hoặc giữa ngắt phần cứng (ISR) và luồng ứng dụng (*Deferred Interrupt Processing*).
   - **Counting Semaphore:** Quản lý tài nguyên hữu hạn (*Resource Pool*) và bộ đệm lưu vết các sự kiện dồn dập.
   - **Mutex (`k_mutex`):** Cơ chế loại trừ tương hỗ (*Mutual Exclusion*), ngăn chặn xung đột tài nguyên (*Race Condition*) với khả năng tự động thừa kế độ ưu tiên (*Priority Inheritance*) chống đảo ngược ưu tiên (*Priority Inversion*).
   - **Message Queue (`k_msgq`):** Hàng đợi vòng truyền nhận dữ liệu có cấu trúc an toàn theo mô hình Producer - Consumer.
2. **Quy chuẩn xử lý ngắt an toàn (ISR Standards):** Tuyệt đối không dùng hàm chặn trong ISR; bắt buộc sử dụng timeout `K_NO_WAIT` và chuyển các tác vụ tính toán nặng sang Thread.
3. **Cơ chế định thời gian chờ (Kernel Timeouts):** Vận dụng linh hoạt các hằng số định thời: `K_FOREVER` (chờ vô hạn), `K_NO_WAIT` (thăm dò tức thì không chờ), `K_MSEC(t)` (chờ có thời hạn).

---

## 📁 2. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab2_synchronization)
├── demos/                              # Các bài thực nghiệm khảo sát cơ chế IPC
│   ├── lab2_demo1_binary_sem/          # Demo 1: Xử lý ngắt trễ với Binary Semaphore
│   ├── lab2_demo2_counting_sem/        # Demo 2: Đệm sự kiện với Counting Semaphore
│   ├── lab2_demo3_mutex/               # Demo 3: Bảo vệ biến chia sẻ chống Race Condition
│   └── lab2_demo4_msgq/                # Demo 4: Truyền nhận cấu trúc lệnh qua Message Queue
├── lab2_ex1_relay_binary_sem/          # Bài 1: Cắt nguồn Rơ-le bảo vệ quá nhiệt khẩn cấp
├── lab2_ex2_timeout_monitor/           # Bài 2: Giám sát Timeout phát hiện cảm biến bị treo
├── lab2_ex3_counting_sem_workers/      # Bài 3: Điều phối cụm luồng (Thread Pool) 3 Worker
├── lab2_ex4_isr_message_queue/         # Bài 4: Chuyển dữ liệu nút bấm từ ISR vào Message Queue
├── lab2_ex5_pedestrian_traffic_light/  # Bài 5: Đèn giao thông ưu tiên người đi bộ qua đường
├── .gitignore
└── README.md                           # Tài liệu kỹ thuật Lab 2
```

---

## 🔌 3. Phân bố chân phần cứng & Devicetree

Toàn bộ tài nguyên ngoại vi được liên kết đồng nhất qua file `app.overlay`:

| Tên ngoại vi | Chân GPIO STM32F407 | Alias Devicetree | Chế độ / Logic hoạt động |
| :--- | :--- | :--- | :--- |
| **LED3 (DEBUG LED)** | `PE4` | `led0` | Output Active-High (Mức 1 = Sáng) |
| **Output Y0** | `PE5` | `led1` | Output Active-High |
| **Output Y1** | `PE6` | `led2` | Output Active-High |
| **Rơ-le công suất** | `PE2` | `relay0` (hoặc direct GPIO) | Output Active-High (Đóng/Ngắt mạch điện) |
| **Nút nhấn (KEY)** | `PB12` | `sw0` (hoặc direct GPIO) | Input Pull-up kèm ngắt sườn xuống (Falling Edge) |
| **Console UART** | `PA9` (TX) / `PA10` (RX) | `usart1` | 115200 bps, 8-N-1 (Serial Log) |

---

## 📊 4. Danh mục các bài thực hành chi tiết

### 4.1. Các bài Demo nền tảng (`demos/`)

- **`lab2_demo1_binary_sem` (Deferred ISR):** Bộ định thời phần cứng `k_timer` tạo ngắt mỗi 2 giây. Callback ngắt chỉ phát tín hiệu `k_sem_give(&evt_sem)`. Luồng ứng dụng chờ tại `k_sem_take(&evt_sem, K_FOREVER)` thức dậy nháy LED3 ba lần, giải phóng hoàn toàn thời gian cho ISR.
- **`lab2_demo2_counting_sem` (Event Buffering):** Timer kích hoạt đợt bùng nổ 15 sự kiện liên tiếp (300 ms/lần) trong khi luồng chỉ xử lý được với tốc độ 1000 ms/sự kiện. Counting Semaphore với giới hạn `EVT_LIMIT = 5` đóng vai trò bộ đệm nhớ các sự kiện chưa kịp phục vụ mà không bị mất dấu.
- **`lab2_demo3_mutex` (Data Integrity):** Hai luồng cùng chạy vòng lặp 1000 lần tăng biến đếm chung `counter`. Khi không có Mutex (`USE_MUTEX = 0`), xảy ra tranh chấp dữ liệu (*Race Condition*) khiến kết quả sai lệch. Khi khóa bằng `k_mutex_lock()`, kết quả luôn đạt chuẩn xác 2000.
- **`lab2_demo4_msgq` (Producer - Consumer):** Luồng Producer đóng gói cấu trúc `struct led_cmd` đẩy vào hàng đợi qua `k_msgq_put()`. Luồng Consumer lấy lệnh qua `k_msgq_get()` và điều khiển đèn tuần tự theo cơ chế FIFO.

---

### 4.2. Các bài tập ứng dụng thực hành (`lab2_ex*`)

#### 🛡️ Bài 1: `lab2_ex1_relay_binary_sem` - Rơ-le bảo vệ quá nhiệt
- **Mục tiêu:** Xây dựng cơ chế an toàn công nghiệp ngắt mạch tức thời khi nhiệt độ vượt ngưỡng.
- **Thực thi:**
  - **Luồng Giám sát:** Đọc cảm biến nhiệt giả lập; khi phát hiện nhiệt độ vượt ngưỡng an toàn ($> 50^\circ\text{C}$), lập tức gọi `k_sem_give(&overheat_sem)`.
  - **Luồng Bảo vệ khẩn cấp:** Bị chặn tại `k_sem_take(&overheat_sem, K_FOREVER)`. Khi nhận tín hiệu, lập tức kéo chân `PE2` xuống ngắt Rơ-le để cô lập tải, đồng thời bật còi/đèn cảnh báo và in log khẩn ra UART.

---

#### ⏱️ Bài 2: `lab2_ex2_timeout_monitor` - Giám sát lỗi Timeout cảm biến
- **Mục tiêu:** Phát hiện sự cố cảm biến bị treo hoặc đứt cáp truyền thông bằng cơ chế Timeout của Semaphore.
- **Thực thi:**
  - **Luồng Cảm biến:** Định kỳ gửi tín hiệu sống còn (Heartbeat) qua Semaphore.
  - **Luồng Giám sát:** Chờ tín hiệu bằng lệnh `k_sem_take(&sensor_sem, K_MSEC(2500))`.
  - Nếu cảm biến không phản hồi trong vòng 2.5 giây, hàm trả về mã lỗi `-EAGAIN` (*Timeout*). Luồng Giám sát lập tức phát thông báo lỗi hệ thống `[TIMEOUT ERROR] Sensor disconnected`.

---

#### 👥 Bài 3: `lab2_ex3_counting_sem_workers` - Quản lý Thread Pool
- **Mục tiêu:** Điều phối tải công việc thông qua hồ tài nguyên hữu hạn (*Resource Pool*).
- **Thực thi:**
  - Khởi tạo Semaphore đếm: `K_SEM_DEFINE(worker_pool_sem, 3, 3);` (Tối đa 3 Worker đồng thời).
  - Khi có công việc mới, bộ điều phối kiểm tra tính khả dụng bằng `k_sem_take(&worker_pool_sem, K_NO_WAIT)`.
  - Nếu Pool còn chỗ, công việc được giao; khi Worker xử lý xong sẽ gọi `k_sem_give(&worker_pool_sem)` để hoàn trả slot phục vụ cho các yêu cầu tiếp theo.

---

#### 📥 Bài 4: `lab2_ex4_isr_message_queue` - Giao tiếp ISR tới Thread
- **Mục tiêu:** Thu thập sự kiện ngắt nút bấm phần cứng an toàn, không làm treo vi điều khiển.
- **Thực thi:**
  - Cấu hình ngắt GPIO Falling Edge trên chân nút nhấn `PB12`.
  - Trong hàm ngắt `button_pressed_isr()`:
    - Tuyệt đối không dùng `k_msleep()` hay in chuỗi `printk()` dài dòng.
    - Đóng gói dữ liệu `struct btn_event` gồm timestamp và trạng thái nút.
    - Đẩy vào hàng đợi bằng `k_msgq_put(&btn_msgq, &evt, K_NO_WAIT)`. Nếu hàng đợi đầy, sự kiện được bỏ qua mà không làm nghẽn CPU.
  - **Luồng Xử lý:** Đón nhận dữ liệu từ hàng đợi qua `k_msgq_get(&btn_msgq, &evt, K_FOREVER)` để đổi chế độ làm việc.

---

#### 🚦 Bài 5: `lab2_ex5_pedestrian_traffic_light` - Đèn giao thông ưu tiên
- **Mục tiêu:** Xây dựng hệ thống đèn giao thông thích ứng, chuyển pha ưu tiên khi người đi bộ ấn nút.
- **Thực thi:**
  - Cụm đèn hoạt động theo chu kỳ FSM thông thường: Đỏ (7s) $\rightarrow$ Xanh (5s) $\rightarrow$ Vàng (2s).
  - Khi người đi bộ bấm nút (`PB12`), ngắt ngoại vi kích hoạt và đẩy cờ sự kiện vào Message Queue.
  - FSM nhận cờ ưu tiên: Rút ngắn ngay lập tức pha Xanh về Vàng, chuyển sang Đỏ cho phương tiện và bật đèn ưu tiên người đi bộ trong 5 giây. Sau đó tự động khôi phục chu trình bình thường.

---

## 💻 5. Hướng dẫn biên dịch & Nạp chương trình

Thực thi lệnh `west` từ thư mục gốc của nhánh `lab2_synchronization`:

```bash
# 1. Biên dịch bài tập cụ thể (Ví dụ: Bài 4 ISR sang Message Queue)
west build -p always -b stm32f4_disco lab2_ex4_isr_message_queue

# 2. Nạp firmware vào Kit BKIT ARM4 qua mạch nạp ST-Link
west flash

# 3. Mở Serial Console giám sát kết quả (115200 bps, 8-N-1)
picocom -b 115200 /dev/ttyUSB0
```

---

## ⚙️ 6. Quy tắc kỹ thuật & Lưu ý cốt lõi

1. **Nguyên tắc bất di bất dịch trong ISR:** Hàm xử lý ngắt phần cứng (ISR) không thuộc ngữ cảnh của bất kỳ thread nào. Tuyệt đối không được gọi các hàm có khả năng gây chặn (`k_msleep`, `k_sem_take` với timeout khác `K_NO_WAIT`, `k_mutex_lock`).
2. **Kế thừa độ ưu tiên của Mutex:** Luôn sử dụng Mutex thay cho Binary Semaphore khi cần bảo vệ vùng nhớ dùng chung, vì Mutex trong Zephyr RTOS hỗ trợ giải thuật **Priority Inheritance** giúp triệt tiêu hiện tượng đảo ngược độ ưu tiên (*Priority Inversion*).
3. **Kích thước phần tử Message Queue:** Cấu hình kích thước phần tử trong `K_MSGQ_DEFINE` phải căn chỉnh theo chuẩn bộ nhớ (bội số của 4 bytes) để tối ưu hóa hiệu năng truy xuất bộ nhớ trên lõi ARM Cortex-M4.
