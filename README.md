# 🧵 LAB 1: ĐA LUỒNG VÀ LẬP LỊCH THỜI GIAN THỰC TRONG ZEPHYR RTOS
### Quản lý luồng tĩnh & động, Phân cấp độ ưu tiên, Stack Allocation & FSM

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (CO3037 / CO3038)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `lab1_multithreading` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Mục tiêu thí nghiệm](#1-mục-tiêu-thí-nghiệm)
- [2. Cấu trúc thư mục dự án](#2-cấu-trúc-thư-mục-dự-án)
- [3. Phân bố chân phần cứng & Devicetree](#3-phân-bố-chân-phần-cứng--devicetree)
- [4. Danh mục các bài thực hành chi tiết](#4-danh-mục-các-bài-thực-hành-chi-tiết)
  - [Bài 0: bkit_blinky - Kiểm tra phần cứng kit](#bài-0-bkit_blinky---kiểm-tra-phần-cứng-kit)
  - [Bài 1: lab1_ex1_asym_blinky - Nháy LED bất đối xứng](#bài-1-lab1_ex1_asym_blinky---nháy-led-bất-đối-xứng)
  - [Bài 2: lab1_ex2_sync_led_y0 - Đồng bộ qua biến dùng chung](#bài-2-lab1_ex2_sync_led_y0---đồng-bộ-qua-biến-dùng-chung)
  - [Bài 3: lab1_ex3_independent_patterns - Mẫu nháy đa luồng độc lập](#bài-3-lab1_ex3_independent_patterns---mẫu-nháy-đa-luồng-độc-lập)
  - [Bài 4: lab1_ex4_multi_led_param - Tái sử dụng Thread bằng con trỏ tham số](#bài-4-lab1_ex4_multi_led_param---tái-sử-dụng-thread-bằng-con-trỏ-tham-số)
  - [Bài 5: lab1_ex5_traffic_light - Hệ thống đèn giao thông FSM đa luồng](#bài-5-lab1_ex5_traffic_light---hệ-thống-đèn-giao-thông-fsm-đa-luồng)
- [5. Hướng dẫn biên dịch & Nạp chương trình](#5-hướng-dẫn-biên-dịch--nạp-chương-trình)
- [6. Quy tắc kỹ thuật & Lưu ý cốt lõi](#6-quy-tắc-kỹ-thuật--lưu-ý-cốt-lõi)

---

## 🎯 1. Mục tiêu thí nghiệm

1. **Làm chủ kiến trúc đa luồng (Multithreading):** Hiểu rõ cách thức kernel của Zephyr RTOS phân bổ thời gian thực thi CPU giữa các luồng độc lập, xóa bỏ hoàn toàn hàm dừng blocking.
2. **Kỹ thuật khởi tạo luồng:**
   - Khởi tạo luồng tĩnh tại thời điểm biên dịch với macro `K_THREAD_DEFINE()`.
   - Khởi tạo luồng động linh hoạt trong thời gian chạy bằng API `k_thread_create()`.
3. **Phân cấp độ ưu tiên (Thread Priority Hierarchy):**
   - **Cooperative Threads (Độ ưu tiên âm: `-15` đến `-1`):** Luồng hợp tác, không bị chiếm quyền bởi bất kỳ luồng nào khác trừ khi chủ động nhường CPU bằng `k_yield()` hoặc chuyển sang trạng thái ngủ (`k_msleep()`).
   - **Preemptive Threads (Độ ưu tiên không âm: `0` đến `14`):** Luồng chiếm quyền, sẵn sàng bị tạm dừng ngay lập tức khi có một luồng có độ ưu tiên cao hơn (giá trị số nhỏ hơn) thức giấc.
4. **Quản trị ngăn xếp (Stack Management):** Cấp phát stack size phù hợp (`K_THREAD_STACK_DEFINE`) và cấu hình cơ chế bảo vệ phần cứng chống tràn stack (`CONFIG_HW_STACK_PROTECTION`).
5. **Truyền tham số cấu hình luồng:** Tái sử dụng hàm thực thi luồng duy nhất bằng con trỏ cấu trúc dữ liệu (`struct blink_cfg`).

---

## 📁 2. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab1_multithreading)
├── bkit_blinky/                        # Ứng dụng kiểm tra phần cứng LED ban đầu
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── app.overlay
│   └── src/main.c
├── lab1_ex1_asym_blinky/               # Bài 1: Luồng nháy LED bất đối xứng
├── lab1_ex2_sync_led_y0/               # Bài 2: Đồng bộ 2 luồng qua biến volatile
├── lab1_ex3_independent_patterns/      # Bài 3: Hai luồng chạy chu kỳ độc lập lệch pha
├── lab1_ex4_multi_led_param/           # Bài 4: Tái sử dụng hàm Thread qua tham số
├── lab1_ex5_traffic_light/             # Bài 5: FSM đèn giao thông kết hợp Heartbeat
├── .gitignore
└── README.md                           # Tài liệu kỹ thuật Lab 1
```

---

## 🔌 3. Phân bố chân phần cứng & Devicetree

Hệ thống sử dụng file Devicetree overlay (`app.overlay`) để trừu tượng hóa các chân ngoại vi trên Kit BKIT ARM4:

| Tên ngoại vi | Chân GPIO STM32F407 | Node Label Devicetree | Chế độ / Logic hoạt động |
| :--- | :--- | :--- | :--- |
| **LED3 (DEBUG LED)** | `PE4` | `led0` | Output Active-High (Mức 1 = Sáng) |
| **Output Y0** | `PE5` | `led1` | Output Active-High |
| **Output Y1** | `PE6` | `led2` | Output Active-High |
| **Console UART** | `PA9` (TX) / `PA10` (RX) | `usart1` | 115200 bps, 8-N-1 (Cổng DB9 MAX232) |

---

## 📊 4. Danh mục các bài thực hành chi tiết

### 💡 Bài 0: `bkit_blinky` - Kiểm tra phần cứng kit
- **Yêu cầu:** Xác nhận chuỗi công cụ toolchain `west`, mạch nạp ST-Link và trạng thái hoạt động của LED3 (`PE4`).
- **Thực thi:** Nháy LED3 chu kỳ đối xứng: Sáng 1000 ms, Tắt 1000 ms.
- **Kiểm chứng:** LED3 trên kit nhấp nháy đều đặn với chu kỳ 2 giây; console UART in thông điệp khởi động thành công.

---

### ⏱️ Bài 1: `lab1_ex1_asym_blinky` - Nháy LED bất đối xứng
- **Yêu cầu:** Tách biệt logic điều khiển ra khỏi hàm `main()` bằng cách khởi tạo luồng độc lập `thread_led_blinky` qua macro `K_THREAD_DEFINE`.
- **Thông số kỹ thuật:**
  - Kích thước ngăn xếp: `1024` bytes.
  - Mức độ ưu tiên: `7` (Preemptive).
  - Chu kỳ: **Sáng 1000 ms (1s)** / **Tắt 2000 ms (2s)** $\rightarrow$ Tổng chu kỳ $T = 3000\text{ ms}$, Duty Cycle $D \approx 33.3\%$.
- **Kiểm chứng:** Hàm `main()` kết thúc ngay lập tức nhưng luồng độc lập vẫn nhấp nháy đèn liên tục theo đúng nhịp định thời.

---

### 🔄 Bài 2: `lab1_ex2_sync_led_y0` - Đồng bộ qua biến dùng chung
- **Yêu cầu:** Phối hợp đồng bộ trạng thái logic thời gian thực giữa 2 luồng độc lập thông qua biến cờ toàn cục.
- **Thiết kế hệ thống:**
  - Biến cờ đồng bộ: `volatile bool g_led_state = false;`.
  - **Luồng Chủ (`thread_led_blinky`):** Điều khiển LED3 nhấp nháy chu kỳ 2s SÁNG / 2s TẮT, đồng thời cập nhật giá trị vào `g_led_state`.
  - **Luồng Phụ (`thread_y0_sync`):** Thăm dò biến `g_led_state` mỗi 10 ms và xuất mức logic tương ứng ra ngõ ra Output Y0 (`PE5`).
- **Lưu ý kỹ thuật:** Bắt buộc sử dụng từ khóa `volatile` để ngăn trình biên dịch tối ưu cất giá trị vào thanh ghi CPU, đảm bảo luồng phụ luôn đọc trực tiếp từ bộ nhớ RAM.

---

### 🔀 Bài 3: `lab1_ex3_independent_patterns` - Mẫu nháy đa luồng độc lập
- **Yêu cầu:** Chứng minh tính độc lập tuyệt đối giữa 2 tác vụ định thời bất đối xứng chạy song song:
  - **Luồng 1 (`thread_led_blinky`):** Điều khiển LED3 nháy Sáng 1500 ms, Tắt 500 ms (Chu kỳ 2000 ms).
  - **Luồng 2 (`thread_y0_y1_pattern`):** Tuần tự điều khiển 2 ngõ ra Y0 và Y1 theo chu kỳ 5000 ms qua 3 pha:
    - Pha 1 (2000 ms): `Y0 = 1, Y1 = 0`.
    - Pha 2 (1500 ms): `Y0 = 0, Y1 = 1`.
    - Pha 3 (1500 ms): `Y0 = 1, Y1 = 1`.
- **Kiểm chứng:** Tín hiệu trên LED3 và cụm ngõ ra Y0/Y1 lệch pha tự nhiên, không làm sai lệch nhịp thời gian của nhau.

---

### 🧩 Bài 4: `lab1_ex4_multi_led_param` - Tái sử dụng Thread bằng con trỏ tham số
- **Yêu cầu:** Không lặp lại mã nguồn; sử dụng duy nhất một hàm luồng `thread_blink_entry` để điều khiển đồng thời cả 3 đèn LED với các chu kỳ khác nhau:
  ```c
  struct blink_cfg {
      struct gpio_dt_spec led;
      int on_ms;
      int off_ms;
      const char *name;
  };
  ```
- **Cấu hình tham số độc lập:**
  - **LED3 (`PE4`):** Sáng 500 ms / Tắt 500 ms.
  - **Output Y0 (`PE5`):** Sáng 1000 ms / Tắt 1000 ms.
  - **Output Y1 (`PE6`):** Sáng 1500 ms / Tắt 1500 ms.
- **Kỹ thuật:** Truyền địa chỉ con trỏ cấu hình (`&cfg_led3`, `&cfg_y0`, `&cfg_y1`) thông qua tham số `p1` của macro `K_THREAD_DEFINE`.

---

### 🚦 Bài 5: `lab1_ex5_traffic_light` - Hệ thống đèn giao thông FSM đa luồng
- **Yêu cầu:** Mô phỏng hệ thống đèn giao thông 3 pha tuần tự kết hợp luồng giám sát sống còn (*Heartbeat Monitor*):
  - **Luồng 1 (Heartbeat):** LED3 nhấp nháy đều đặn 1000 ms ON / 1000 ms OFF báo hiệu hệ vi điều khiển đang hoạt động bình thường.
  - **Luồng 2 (FSM Giao thông):** Máy trạng thái điều khiển cụm đèn Y0 và Y1:
    - 🔴 **Pha ĐỎ (Y0):** Sáng duy trì **7000 ms** (7 giây).
    - 🟢 **Pha XANH (Y1):** Sáng duy trì **5000 ms** (5 giây).
    - 🟡 **Pha VÀNG (Bật cả Y0 và Y1):** Sáng duy trì **2000 ms** (2 giây).
- **Kiểm chứng:** FSM chuyển pha mượt mà, đúng chuẩn thời gian thực mà không làm gián đoạn nhịp đập của luồng Heartbeat.

---

## 💻 5. Hướng dẫn biên dịch & Nạp chương trình

Từ thư mục gốc của nhánh `lab1_multithreading`, thực hiện các lệnh quản lý với `west`:

```bash
# 1. Biên dịch một bài tập cụ thể (Ví dụ: Bài 5 Đèn giao thông)
west build -p always -b stm32f4_disco lab1_ex5_traffic_light

# 2. Nạp firmware vào Kit BKIT ARM4 thông qua mạch nạp ST-Link
west flash

# 3. Mở Serial Terminal giám sát nhật ký UART (115200 bps, 8-N-1)
# Trên Linux/WSL:
picocom -b 115200 /dev/ttyUSB0
# Hoặc trên Windows sử dụng PuTTY / Tera Term chọn đúng cổng COM tương ứng
```

---

## ⚙️ 6. Quy tắc kỹ thuật & Lưu ý cốt lõi

1. **Cấp phát ngăn xếp an toàn:** Hàm `printk()` và driver GPIO tiêu tốn bộ nhớ ngăn xếp; mỗi luồng phải được cấp phát tối thiểu **1024 bytes** stack (`1024`) để phòng ngừa sự cố tràn ngăn xếp gây lỗi phần cứng (*HardFault Exception*).
2. **Quy hoạch độ ưu tiên:** Các luồng ứng dụng thông thường nên cấu hình trong dải ưu tiên từ `5` đến `8` (Preemptive). Luồng có yêu cầu thời gian thực khắt khe hơn cần đặt mức ưu tiên cao hơn (giá trị số nhỏ hơn).
3. **Tránh bẫy Cooperative Lock-up:** Nếu cấu hình luồng có độ ưu tiên âm (Cooperative, ví dụ: `-1`), luồng đó sẽ chiếm giữ CPU liên tục và bộ lập lịch sẽ không thể ngắt để chuyển sang luồng khác trừ khi luồng chủ động gọi `k_msleep()` hoặc `k_yield()`.
