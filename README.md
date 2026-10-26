# LAB 3: MẠNG TRUYỀN THÔNG CAN BUS VÀ HỆ THỐNG PHÂN TÁN

Khoa Khoa học và Kỹ thuật Máy tính - Trường Đại học Bách Khoa, ĐHQG-HCM  
Môn học: Hệ thống nhúng (CO3037 / CO3038)  
Nhánh Git: `lab3_canbus`  
Nền tảng: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
Hệ điều hành: Zephyr RTOS v4.4.0  

---

## 1. Mục tiêu thí nghiệm

1. Nắm vững kiến trúc và nguyên lý truyền thông mạng vi điều khiển công nghiệp Controller Area Network (CAN Bus 2.0A/B theo tiêu chuẩn ISO 11898):
   - Cơ chế phân xử đường truyền không phá hủy (Non-destructive Bitwise Arbitration) dựa trên Identifier.
   - Cấu trúc các loại khung CAN: Khung dữ liệu (Data Frame), Khung yêu cầu từ xa (Remote Transmission Request - RTR Frame), Khung báo lỗi (Error Frame).
   - Bộ điều khiển CAN STM32 bxCAN: FIFO RX, Mailbox TX, Bit Timing (Prescaler, Phase Segments, SJW) đạt tốc độ chuẩn 250 kbps hoặc 500 kbps.
2. Thiết lập bộ lọc phần cứng (Hardware Acceptance Filters & Masks) bằng API `can_add_rx_filter()` của Zephyr để giảm tải tối đa cho CPU.
3. Chuyển đổi linh hoạt giữa chế độ kiểm thử nội bộ (Loopback Mode) và chế độ mạng thực tế đa node (Normal Bus Mode).
4. Thiết kế hệ thống phân tán: Giám sát sống còn (Heartbeat Monitoring) và hệ thống đèn giao thông đồng bộ giữa 2 Kit Master - Slave.

---

## 2. Phân bố chân phần cứng và Sơ đồ đấu nối (Hardware Setup)

### 2.1. Cấu hình chân CAN1 trên STM32F407
File `app.overlay` gán các chân Alternate Function cho ngoại vi CAN1:

| Tín hiệu | Chân GPIO STM32F407 | Chức năng Alternate Function |
| :--- | :--- | :--- |
| CAN1_RX | `PB8` | AF9 (CAN1) |
| CAN1_TX | `PB9` | AF9 (CAN1) |
| LED3 / Output Y0 / Y1 | `PE4` / `PE5` / `PE6` | Ngõ ra chỉ thị trạng thái |
| Console UART | `PA9` / `PA10` | USART1 (115200 bps) |

### 2.2. Sơ đồ đấu nối giữa 2 Kit BKIT ARM4 (Giao tiếp Bus thật)
Khi chạy các bài thực hành giữa 2 Kit phần cứng (`lab3_demo3`, `lab3_ex4`, `lab3_ex5`), đấu nối qua bộ thu phát CAN Transceiver (TJA1050 hoặc SN65HVD230):

```
       KIT A (Master / Node 1)                 KIT B (Slave / Node 2)
  +-------------------------------+       +-------------------------------+
  |  STM32F407      CAN Trcv      |       |  STM32F407      CAN Trcv      |
  |  PB9 (TX)  ---> TXD           |       |  PB9 (TX)  ---> TXD           |
  |  PB8 (RX)  <--- RXD           |       |  PB8 (RX)  <--- RXD           |
  |                               |       |                               |
  |             [CAN_H] ----------+-------+---------- [CAN_H]             |
  |                |              |       |              |                |
  |               [120R]          |  CAN  |             [120R]            |
  |                |              |  BUS  |              |                |
  |             [CAN_L] ----------+-------+---------- [CAN_L]             |
  |                               |       |                               |
  |             [GND] ------------+-------+---------- [GND]               |
  +-------------------------------+       +-------------------------------+
```

**Lưu ý kỹ thuật bắt buộc:**
- Phải gắn điện trở đầu cuối 120 Ohm (Terminating Resistor) ở cả 2 đầu đường truyền vi sai để tránh hiện tượng phản xạ sóng tín hiệu.
- Bắt buộc phải nối chung đường Mass (GND) giữa 2 board nhằm xác lập mức điện thế tham chiếu ổn định cho CANH và CANL.

---

## 3. Danh mục các bài thực hành và Mô tả chi tiết

### 3.1. Các bài Demo nền tảng (`demos/`)

- `lab3_demo1_loopback`:
  - Kích hoạt chế độ `CAN_MODE_LOOPBACK`.
  - Node tự đóng gói và gửi khung CAN ID `0x123` gồm 4 byte payload; sau đó tự thu lại qua bộ đệm RX để kiểm tra tính toàn vẹn của stack mạng Zephyr.
- `lab3_demo2_filters`:
  - Cấu hình bộ lọc phần cứng linh hoạt:
    - Filter 1: Bắt chính xác ID `0x100` với Mask tuyệt đối `0x7FF`.
    - Filter 2: Bắt nhóm ID từ `0x200` đến `0x20F` với Mask dải `0x7F0`.
  - Chứng minh các khung có ID không khớp với bộ lọc sẽ bị phần cứng hủy ngay lập tức mà không tiêu tốn tài nguyên ngắt của CPU.
- `lab3_demo3_two_nodes`:
  - Giao tiếp giữa 2 kit trên đường truyền thực tế (`CAN_MODE_NORMAL`).
  - Kit A mang `NODE_ID = 1`, Kit B mang `NODE_ID = 2`.
  - Mỗi giây mỗi node phát Heartbeat ID `0x700 + NODE_ID` và nhận Heartbeat của đối phương; đồng thời theo dõi trạng thái lỗi bus qua `can_get_state()`.

---

### 3.2. Các bài tập ứng dụng (`lab3_ex*`)

#### Bài 1: `lab3_ex1_can_sensor_loopback` - Đóng gói và Giải mã dữ liệu cảm biến
- **Yêu cầu:** Mô phỏng thu thập dữ liệu đa cảm biến và truyền qua mạng CAN Loopback.
- **Định dạng khung dữ liệu (Payload 8 bytes, ID: 0x200):**
  - Byte 0-1: Giá trị Nhiệt độ (`int16_t`, đơn vị: 0.1°C).
  - Byte 2: Độ ẩm tương đối (`uint8_t`, %).
  - Byte 3-4: Áp suất khí quyển (`uint16_t`, hPa).
  - Byte 5: Mã trạng thái hệ thống (`uint8_t`).
  - Byte 6-7: Checksum / Counter tăng dần.
- **Thực thi:** Đóng gói đúng chuẩn Byte-order (Little Endian), truyền qua CAN, giải mã tại callback nhận và in thông số ra UART console.

#### Bài 2: `lab3_ex2_can_command_rtr` - Điều khiển lệnh và Khung yêu cầu từ xa (RTR)
- **Yêu cầu:** Thực hiện cơ chế hỏi/đáp trạng thái không cần gửi kèm dữ liệu tải.
- **Thiết kế:**
  - Node Master gửi một khung RTR Frame (`flags = CAN_FRAME_RTR`, `dlc = 0`, ID: `0x301`).
  - Node Slave cấu hình bộ lọc bắt RTR Frame trên ID `0x301`. Khi nhận được yêu cầu, Slave tự động phản hồi lại một Data Frame chứa trạng thái hiện tại của Rơ-le và các ngõ ra Y0/Y1.
  - Đồng thời Master có thể gửi Data Frame mang ID `0x300` chứa lệnh bật/tắt thiết bị trực tiếp.

#### Bài 3: `lab3_ex3_can_filter_priority` - Bộ lọc phần cứng và Phân xử ưu tiên ID
- **Yêu cầu:** Thực nghiệm cơ chế phân xử bitwise arbitration không phá hủy.
- **Thiết kế:**
  - Hai luồng phát đồng thời 2 khung CAN vào cùng một thời điểm:
    - Gói tin Khẩn cấp (Alarm/Emergency): ID `0x080` (Ưu tiên cao).
    - Gói tin Định kỳ (Telemetry): ID `0x400` (Ưu tiên thấp).
  - Kết quả thực nghiệm: Khung ID `0x080` luôn giành quyền đường truyền và hoàn tất trước mà không làm hỏng gói tin ID thấp hơn.
  - Cấu hình bộ lọc Mask phần cứng để chỉ tiếp nhận các bản tin hợp lệ.

#### Bài 4: `lab3_ex4_can_heartbeat_ping` - Giám sát sống còn Ping-Pong 2 Node
- **Yêu cầu:** Giám sát trạng thái kết nối vật lý trên bus CAN giữa 2 kit độc lập.
- **Thiết kế:**
  - Node 1 (Kit A) định kỳ 1000 ms phát bản tin Heartbeat ID `0x701`.
  - Node 2 (Kit B) khởi tạo timer giám sát 2500 ms:
    - Mỗi khi nhận được Heartbeat từ Kit A, timer được nạp lại.
    - Nếu Kit A mất nguồn hoặc đứt dây CAN, timer hết hạn, Kit B lập tức kích hoạt LED cảnh báo "NODE 1 TIMEOUT / OFFLINE".
  - Chức năng mở rộng: Giao thức Ping-Pong đo thời gian trễ khứ hồi (Round-Trip Time - RTT) qua CAN bus.

#### Bài 5: `lab3_ex5_distributed_traffic_light` - Hệ thống đèn giao thông phân tán qua CAN
- **Yêu cầu:** Xây dựng hệ thống đèn giao thông thông minh phân tán trên 2 Kit:
  - **Kit A (Controller Node - Master):**
    - Chạy FSM giao thông (Đỏ 7s, Xanh 5s, Vàng 2s).
    - Tính toán thời gian đếm ngược thực tế mỗi giây.
    - Đóng gói trạng thái: `{ uint8_t phase_id; uint8_t remaining_sec; }` và broadcast lên bus CAN với ID `0x250`.
  - **Kit B (Display Node - Slave):**
    - Đặt tại hiện trường, nhận khung CAN ID `0x250`.
    - Đồng bộ hiển thị LED3, Y0, Y1 và in thông tin đếm ngược ra cổng UART.
    - Cơ chế an toàn (Fail-safe): Nếu mất kết nối CAN với Master quá 3 giây, Slave tự động chuyển sang chế độ nháy Đèn Vàng cảnh báo nguy hiểm.

---

## 4. Hướng dẫn Biên dịch và Nạp chương trình

### 4.1. Chạy chế độ Loopback trên 1 Kit
```bash
west build -p always -b stm32f4_disco lab3_ex1_can_sensor_loopback
west flash
```

### 4.2. Chạy chế độ 2 Kit trên Bus thật
1. **Nạp cho Kit A (Node 1 / Master):**
   - Mở file `main.c`, xác nhận định nghĩa `#define NODE_ID 1`.
   - Kết nối ST-Link với Kit A và nạp:
     ```bash
     west build -p always -b stm32f4_disco lab3_ex5_distributed_traffic_light
     west flash
     ```

2. **Nạp cho Kit B (Node 2 / Slave):**
   - Mở file `main.c`, đổi định nghĩa `#define NODE_ID 2`.
   - Kết nối ST-Link sang Kit B và nạp:
     ```bash
     west build -p always -b stm32f4_disco lab3_ex5_distributed_traffic_light
     west flash
     ```

3. **Kiểm tra:** Mở 2 cửa sổ Serial Monitor cho cả 2 kit ở tốc độ 115200 bps để giám sát hoạt động đồng bộ.

---

## 5. Quy tắc kỹ thuật và Lưu ý về CAN Bus

1. **Cấu hình Kconfig bắt buộc:** Trong `prj.conf`, luôn cần có `CONFIG_CAN=y` và `CONFIG_CAN_MAX_FILTER=5` để kích hoạt driver CAN trên STM32.
2. **Chế độ Bus Mode:** Không thể truyền nhận giữa 2 kit nếu một bên đang bật `CAN_MODE_LOOPBACK`. Phải chuyển sang `CAN_MODE_NORMAL` (hoặc đặt flags = 0).
3. **Lỗi Bus-Off:** Nếu dây truyền tín hiệu bị hở hoặc thiếu trở 120 Ohm, bộ điều khiển CAN sẽ tích lũy lỗi và chuyển sang trạng thái `CAN_STATE_BUS_OFF`. Cần kiểm tra kỹ phần cứng trước khi chạy thực nghiệm.
