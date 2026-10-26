# LAB 2: ĐỒNG BỘ TIẾN TRÌNH VÀ TRUYỀN THÔNG LIÊN TIẾN TRÌNH (IPC) TRONG ZEPHYR RTOS

Khoa Khoa học và Kỹ thuật Máy tính - Trường Đại học Bách Khoa, ĐHQG-HCM  
Môn học: Hệ thống nhúng (CO3037 / CO3038)  
Nhánh Git: `lab2_synchronization`  
Nền tảng: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
Hệ điều hành: Zephyr RTOS v4.4.0  

---

## 1. Mục tiêu thí nghiệm

1. Nắm vững các cơ chế đồng bộ và truyền thông liên tiến trình (IPC) trong hệ điều hành thời gian thực Zephyr RTOS:
   - **Binary Semaphore (`k_sem`):** Đồng bộ hóa 1-1 giữa ngắt phần cứng (ISR) và luồng thực thi (Deferred Interrupt Processing).
   - **Counting Semaphore:** Quản lý hồ chứa tài nguyên hữu hạn (Resource Pool) và bộ đệm đếm sự kiện dồn dập.
   - **Mutex (`k_mutex`):** Loại trừ tương hỗ (Mutual Exclusion), ngăn ngừa xung đột dữ liệu (Race Condition) và cơ chế thừa kế độ ưu tiên (Priority Inheritance).
   - **Message Queue (`k_msgq`):** Hàng đợi vòng truyền nhận dữ liệu có cấu trúc an toàn theo mô hình Producer - Consumer.
2. Xử lý đúng quy chuẩn ngắt ngoại vi (GPIO ISR): Không chặn (block) trong ISR, chỉ sử dụng timeout `K_NO_WAIT` và chuyển tác vụ nặng sang Thread.
3. Hiểu rõ cơ chế định thời với Timeout trong kernel (`K_FOREVER`, `K_NO_WAIT`, `K_MSEC`).

---

## 2. Phân bố chân phần cứng (Pinout Configuration)

| Ngoại vi / Tín hiệu | Chân GPIO STM32F407 | Alias Devicetree | Chế độ hoạt động |
| :--- | :--- | :--- | :--- |
| LED3 (DEBUG LED) | `PE4` | `led0` | GPIO Output, Active-High |
| Output Y0 | `PE5` | `led1` | GPIO Output, Active-High |
| Output Y1 | `PE6` | `led2` | GPIO Output, Active-High |
| Rơ-le công suất (Relay) | `PE2` | `relay0` (hoặc direct GPIO) | GPIO Output, Active-High |
| Nút nhấn ngoại vi (KEY) | `PB12` | `sw0` (hoặc direct GPIO) | GPIO Input, Interrupt Edge Falling, Pull-up |
| Console Debug | `PA9` / `PA10` | `usart1` | Baudrate 115200 bps, 8-N-1 |

---

## 3. Danh mục các bài thực hành và Mô tả chi tiết

### 3.1. Các bài Demo nền tảng (`demos/`)

- `lab2_demo1_binary_sem`:
  - Kernel timer (`k_timer`) tạo ngắt chu kỳ mỗi 2 giây.
  - Hàm callback ngắt chỉ thực hiện `k_sem_give(&evt_sem)`.
  - Luồng xử lý chờ `k_sem_take(&evt_sem, K_FOREVER)` và thực thi nháy LED3 3 lần.
  - Minh họa kỹ thuật xử lý ngắt trì hoãn (Deferred Interrupt Processing).
- `lab2_demo2_counting_sem`:
  - Timer kích hoạt đợt bùng nổ 15 sự kiện liên tiếp (cách nhau 300 ms), trong khi luồng chỉ xử lý được với tốc độ 1000 ms/sự kiện.
  - Counting Semaphore với giới hạn `EVT_LIMIT = 5` đóng vai trò bộ đệm ghi nhớ các sự kiện chưa kịp xử lý.
- `lab2_demo3_mutex`:
  - 2 luồng cùng thực thi vòng lặp 1000 lần để tăng một biến đếm chung `counter`.
  - Khi `USE_MUTEX = 0`: Xảy ra tranh chấp tài nguyên (Race Condition), kết quả cuối cùng không đạt 2000.
  - Khi `USE_MUTEX = 1`: `k_mutex_lock()` và `k_mutex_unlock()` đảm bảo tính toàn vẹn dữ liệu, kết quả luôn chính xác 2000.
- `lab2_demo4_msgq`:
  - Luồng Producer đóng gói cấu trúc `struct led_cmd` (chứa chỉ số đèn và thời gian sáng) đẩy vào hàng đợi qua `k_msgq_put()`.
  - Luồng Consumer đọc lệnh qua `k_msgq_get()` và điều khiển đèn theo thứ tự vào trước ra trước (FIFO).

---

### 3.2. Các bài tập ứng dụng (`lab2_ex*`)

#### Bài 1: `lab2_ex1_relay_binary_sem` - Điều khiển Rơ-le bảo vệ quá nhiệt
- **Yêu cầu:** Mô phỏng hệ thống an toàn công nghiệp ngắt mạch tức thời khi quá nhiệt.
- **Thiết kế:**
  - Luồng 1 (Sensor Monitor): Đọc cảm biến nhiệt giả lập. Khi phát hiện nhiệt độ vượt ngưỡng an toàn (> 50°C), lập tức phát tín hiệu `k_sem_give(&overheat_sem)`.
  - Luồng 2 (Safety Protection Thread): Bị chặn tại `k_sem_take(&overheat_sem, K_FOREVER)`. Ngay khi nhận được tín hiệu, lập tức ngắt Rơ-le (`PE2`) để cắt nguồn tải, đồng thời kích hoạt đèn cảnh báo và in log khẩn cấp ra UART.

#### Bài 2: `lab2_ex2_timeout_monitor` - Giám sát lỗi Timeout cảm biến
- **Yêu cầu:** Phát hiện sự cố cảm biến bị treo hoặc mất kết nối thông qua cơ chế Semaphore timeout.
- **Thiết kế:**
  - Luồng Sensor: Định kỳ gửi tín hiệu sống còn (Heartbeat Ping) qua Semaphore.
  - Luồng Monitor: Chờ tín hiệu bằng lệnh `k_sem_take(&sensor_sem, K_MSEC(2500))`.
  - Nếu cảm biến không phản hồi trong vòng 2.5 giây, hàm trả về `-EAGAIN` (Timeout). Luồng Monitor kích hoạt cảnh báo lỗi hệ thống.

#### Bài 3: `lab2_ex3_counting_sem_workers` - Quản lý Thread Pool bằng Counting Semaphore
- **Yêu cầu:** Xây dựng mô hình Pool gồm 3 Worker Threads thực thi các tác vụ tính toán độc lập.
- **Thiết kế:**
  - Khởi tạo `K_SEM_DEFINE(worker_pool_sem, 3, 3)`.
  - Luồng Dispatcher khi tiếp nhận công việc mới sẽ kiểm tra tính khả dụng của pool bằng `k_sem_take(&worker_pool_sem, K_NO_WAIT)`.
  - Nếu pool còn tài nguyên, công việc được phân phối; khi worker hoàn tất nhiệm vụ sẽ gọi `k_sem_give(&worker_pool_sem)` để hoàn trả quyền phục vụ.

#### Bài 4: `lab2_ex4_isr_message_queue` - Giao tiếp ISR tới Thread qua Message Queue
- **Yêu cầu:** Xử lý ngắt nút nhấn phần cứng an toàn, không gây trễ hệ thống.
- **Thiết kế:**
  - Cấu hình ngắt GPIO Falling Edge trên chân nút nhấn `PB12`.
  - Trong hàm ngắt `button_pressed_isr()`:
    - Tuyệt đối không dùng `k_msleep` hoặc thao tác in chuỗi dài.
    - Đóng gói sự kiện `struct btn_event` gồm timestamp và trạng thái nút.
    - Gọi `k_msgq_put(&btn_msgq, &evt, K_NO_WAIT)`. Nếu hàng đợi đầy, sự kiện được bỏ qua mà không làm treo MCU.
  - Luồng Worker: Chờ dữ liệu từ hàng đợi bằng `k_msgq_get(&btn_msgq, &evt, K_FOREVER)` để xử lý chuyển đổi chế độ hoạt động.

#### Bài 5: `lab2_ex5_pedestrian_traffic_light` - Đèn giao thông ưu tiên người đi bộ
- **Yêu cầu:** Hệ thống đèn giao thông hoạt động theo chu kỳ FSM (Đỏ 7s, Xanh 5s, Vàng 2s). Khi người đi bộ bấm nút (`PB12`), hệ thống lập tức ưu tiên chuyển pha an toàn.
- **Thiết kế:**
  - Ngắt nút nhấn đưa tín hiệu vào Message Queue hoặc Binary Semaphore.
  - Luồng FSM kiểm tra sự kiện ưu tiên:
    - Khi phát hiện yêu cầu qua đường, chu kỳ Đèn Xanh lập tức rút ngắn về Vàng và chuyển sang Đỏ cho xe cộ.
    - Kích hoạt đèn ưu tiên người đi bộ qua đường trong 5 giây.
    - Sau khi hoàn tất pha ưu tiên, hệ thống tự động khôi phục chu trình giao thông bình thường.

---

## 4. Hướng dẫn Biên dịch và Nạp chương trình

Từ thư mục gốc của nhánh `lab2_synchronization`:

```bash
# Biên dịch Bài 4: ISR sang Thread qua Message Queue
west build -p always -b stm32f4_disco lab2_ex4_isr_message_queue

# Nạp vào kit BKIT ARM4
west flash

# Giám sát kết quả trên Serial Console
picocom -b 115200 /dev/ttyUSB0
```

---

## 5. Quy tắc kỹ thuật và Lưu ý cốt lõi

1. **Nguyên tắc bất di bất dịch trong ISR:**
   - Tuyệt đối không được gọi `k_mutex_lock()` bên trong ISR (sẽ gây lỗi kernel panic vì ISR không thể bị block).
   - Không được gọi `k_sem_take()` với timeout khác `K_NO_WAIT`.
   - Hàm `k_msgq_put()` trong ISR chỉ được phép sử dụng timeout là `K_NO_WAIT`.
2. **Priority Inversion và Mutex:** Khi hai luồng có độ ưu tiên chênh lệch cùng truy cập vào tài nguyên chung, luôn sử dụng `k_mutex` thay vì `k_sem` để Zephyr tự động kích hoạt cơ chế Priority Inheritance ngăn ngừa hiện tượng đảo ngược độ ưu tiên.
