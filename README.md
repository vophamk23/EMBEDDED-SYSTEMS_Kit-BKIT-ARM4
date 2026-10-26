# LAB 1: ĐA LUỒNG VÀ LẬP LỊCH THỜI GIAN THỰC TRONG ZEPHYR RTOS

Khoa Khoa học và Kỹ thuật Máy tính - Trường Đại học Bách Khoa, ĐHQG-HCM  
Môn học: Hệ thống nhúng (CO3037 / CO3038)  
Nhánh Git: `lab1_multithreading`  
Nền tảng: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
Hệ điều hành: Zephyr RTOS v4.4.0  

---

## 1. Mục tiêu thí nghiệm

1. Nắm vững kiến trúc đa luồng (Multithreading) và bộ lập lịch thời gian thực của Zephyr RTOS.
2. Phân biệt cơ chế khai báo luồng tĩnh (`K_THREAD_DEFINE`) và khai báo luồng động (`k_thread_create`).
3. Hiểu rõ cơ chế phân cấp độ ưu tiên:
   - **Cooperative threads:** Độ ưu tiên mang giá trị âm (từ -15 đến -1), không thể bị chiếm quyền (non-preemptible) trừ khi chủ động nhường CPU bằng `k_yield()` hoặc chuyển sang trạng thái ngủ (`k_msleep()`).
   - **Preemptive threads:** Độ ưu tiên mang giá trị không âm (từ 0 đến 14), có thể bị luồng có độ ưu tiên cao hơn ngắt và chiếm quyền thực thi tại bất kỳ thời điểm nào.
4. Thiết lập bộ nhớ ngăn xếp (Stack allocation) và cấu hình bảo vệ chống tràn stack (`CONFIG_STACK_SENTINEL` / `CONFIG_HW_STACK_PROTECTION`).
5. Áp dụng kỹ thuật truyền tham số cấu hình cho luồng (`struct blink_cfg`) và điều khiển máy trạng thái hữu hạn (FSM) kết hợp đa luồng độc lập.

---

## 2. Phân bố chân phần cứng (Pinout Configuration)

Hệ thống sử dụng file Devicetree overlay (`app.overlay`) để cấu hình ngoại vi trên kit BKIT ARM4:

| Ngoại vi / Tín hiệu | Chân GPIO STM32F407 | Alias Devicetree | Chế độ hoạt động |
| :--- | :--- | :--- | :--- |
| LED3 (DEBUG LED) | `PE4` | `led0` | GPIO Output, Active-High |
| Output Y0 | `PE5` | `led1` | GPIO Output, Active-High |
| Output Y1 | `PE6` | `led2` | GPIO Output, Active-High |
| Console Debug | `PA9` / `PA10` | `usart1` | Baudrate 115200 bps, 8-N-1 |

---

## 3. Danh mục các bài thực hành và Mô tả chi tiết

### 3.1. Các bài Demo nền tảng (`demos/`)

- `lab1_demo1_single_thread`:
  - Khảo sát luồng mặc định `main`.
  - Sử dụng `k_msleep()` để định thời và in thông điệp gỡ lỗi ra cổng UART.
- `lab1_demo2_multi_thread`:
  - Khởi tạo 2 luồng cùng chạy song song với cùng một mức độ ưu tiên.
  - Quan sát hành vi chia sẻ thời gian CPU thông qua Time-slicing và gọi chủ động `k_yield()`.
- `lab1_demo3_priority`:
  - Khởi tạo các luồng với các mức độ ưu tiên khác nhau.
  - Chứng minh nguyên tắc: Luồng có giá trị priority nhỏ hơn (ưu tiên cao hơn) sẽ luôn được cấp CPU trước.
- `lab1_demo4_preemption`:
  - Kiểm chứng cơ chế chiếm quyền (Preemptive Scheduling).
  - Khi luồng ưu tiên cao thức giấc (wake up), bộ lập lịch lập tức tạm dừng luồng ưu tiên thấp để phục vụ luồng ưu tiên cao.

---

### 3.2. Các bài tập ứng dụng (`lab1_ex*`)

#### Bài 0: `bkit_blinky` - Kiểm tra phần cứng kit
- **Yêu cầu:** Kiểm tra hoạt động cơ bản của GPIO port E và LED3 trên bo mạch.
- **Thực thi:** Nháy LED3 với chu kỳ đối xứng: Sáng 1000 ms, Tắt 1000 ms.
- **Kiểm chứng:** LED3 trên kit nhấp nháy đều đặn với chu kỳ 2 giây.

#### Bài 1: `lab1_ex1_asym_blinky` - Nháy LED bất đối xứng
- **Yêu cầu:** Tạo một luồng độc lập `thread_led_blinky` bằng macro `K_THREAD_DEFINE`.
- **Thông số kỹ thuật:**
  - Kích thước ngăn xếp (Stack size): 1024 bytes.
  - Độ ưu tiên (Priority): 7 (Preemptive).
  - Chu kỳ nháy: Sáng 1000 ms (1 giây), Tắt 2000 ms (2 giây). Tổng chu kỳ là 3 giây.
- **Mục đích:** Tách biệt logic điều khiển thời gian thực ra khỏi hàm `main()` vào một luồng chuyên biệt.

#### Bài 2: `lab1_ex2_sync_led_y0` - Đồng bộ qua biến dùng chung
- **Yêu cầu:** Phối hợp hoạt động giữa 2 luồng thông qua biến toàn cục `volatile bool g_led_state`.
- **Thiết kế:**
  - Luồng Chủ (`thread_led_blinky`): Điều khiển LED3 nháy chu kỳ Sáng 2000 ms / Tắt 2000 ms; đồng thời cập nhật trạng thái logic vào biến `g_led_state`.
  - Luồng Phụ (`thread_y0_sync`): Chu kỳ lấy mẫu 10 ms đọc biến `g_led_state` và xuất mức logic tương ứng ra Output Y0 (`PE5`).
- **Lưu ý kỹ thuật:** Từ khóa `volatile` bắt buộc phải có để trình biên dịch không tối ưu hóa biến vào thanh ghi, đảm bảo luồng Phụ luôn đọc trực tiếp từ bộ nhớ RAM.

#### Bài 3: `lab1_ex3_independent_patterns` - Mẫu nháy độc lập đa luồng
- **Yêu cầu:** Xây dựng 2 luồng chạy hoàn toàn độc lập với chu kỳ và pha không đồng bộ:
  - Luồng 1 (`thread_led_blinky`): LED3 nháy Sáng 1500 ms, Tắt 500 ms (chu kỳ 2000 ms).
  - Luồng 2 (`thread_y0_y1_pattern`): Điều khiển 2 ngõ ra Y0 và Y1 tuần tự theo chu kỳ 5000 ms qua 3 pha:
    - Pha 1 (2000 ms): Y0 = 1, Y1 = 0.
    - Pha 2 (1500 ms): Y0 = 0, Y1 = 1.
    - Pha 3 (1500 ms): Y0 = 1, Y1 = 1.
- **Kiểm chứng:** Sử dụng Logic Analyzer hoặc hệ thống đèn ngoại vi để kiểm tra tính độc lập tuyệt đối giữa 2 bộ định thời.

#### Bài 4: `lab1_ex4_multi_led_param` - Tái sử dụng hàm Thread bằng tham số
- **Yêu cầu:** Không lặp lại mã nguồn; sử dụng một hàm thực thi duy nhất `thread_blink_entry` để điều khiển đồng thời 3 đèn thông qua con trỏ cấu trúc:
  ```c
  struct blink_cfg {
      struct gpio_dt_spec led;
      int on_ms;
      int off_ms;
      const char *name;
  };
  ```
- **Thông số cấu hình:**
  - LED3 (`PE4`): Sáng 500 ms, Tắt 500 ms.
  - Output Y0 (`PE5`): Sáng 1000 ms, Tắt 1000 ms.
  - Output Y1 (`PE6`): Sáng 1500 ms, Tắt 1500 ms.
- **Kỹ thuật:** Khởi tạo luồng bằng `K_THREAD_DEFINE` và truyền địa chỉ con trỏ `&cfg_led3`, `&cfg_y0`, `&cfg_y1` qua tham số `p1`.

#### Bài 5: `lab1_ex5_traffic_light` - Hệ thống đèn giao thông đa luồng
- **Yêu cầu:** Mô phỏng hệ thống đèn giao thông 3 pha tuần tự kết hợp luồng giám sát sống còn (Heartbeat):
  - Luồng 1 (Heartbeat): Điều khiển LED3 nháy đều đặn 1000 ms ON / 1000 ms OFF nhằm báo hiệu MCU đang vận hành bình thường.
  - Luồng 2 (Máy trạng thái FSM giao thông):
    - Trạng thái RED (Đỏ - Đèn Y0): Sáng 7000 ms.
    - Trạng thái GREEN (Xanh - Đèn Y1): Sáng 5000 ms.
    - Trạng thái YELLOW (Vàng - Bật cả Y0 và Y1): Sáng 2000 ms.
- **Kiểm chứng:** Chu kỳ đèn giao thông chuyển pha chính xác theo thời gian thực mà không làm gián đoạn nhịp đập của Heartbeat.

---

## 4. Hướng dẫn Biên dịch và Nạp chương trình

Từ thư mục gốc của nhánh `lab1_multithreading`, thực hiện các lệnh sau:

```bash
# Biên dịch và nạp Bài 5: Hệ thống đèn giao thông
west build -p always -b stm32f4_disco lab1_ex5_traffic_light

# Nạp firmware qua ST-Link
west flash

# Mở cổng UART để giám sát log (trên Linux)
picocom -b 115200 /dev/ttyUSB0
```

---

## 5. Quy tắc kỹ thuật và Lưu ý

1. **Kích thước Stack:** Do hàm `printk()` và driver GPIO yêu cầu ngăn xếp tương đối, mỗi luồng cần được cấp phát tối thiểu 1024 bytes (`1024`) để tránh lỗi HardFault do tràn ngăn xếp.
2. **Dải độ ưu tiên:** Các luồng thông thường trong bài thực hành được cấu hình trong khoảng từ 5 đến 8 (Preemptive). Luồng có yêu cầu thời gian thực khắt khe hơn cần đặt mức ưu tiên cao hơn (giá trị số nhỏ hơn).
3. **Cooperative vs Preemptive:** Nếu đặt độ ưu tiên âm (ví dụ: `-1`), luồng sẽ chiếm giữ CPU liên tục và bộ lập lịch không thể ngắt trừ khi luồng chủ động nhường quyền (`k_msleep` hoặc `k_yield`).
