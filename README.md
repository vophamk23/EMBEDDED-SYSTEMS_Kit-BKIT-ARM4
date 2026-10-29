# 🚗 LAB 3: MẠNG TRUYỀN THÔNG CAN BUS VÀ HỆ THỐNG PHÂN TÁN
### Giao thức CAN 2.0A/B, Bộ lọc phần cứng STM32 bxCAN, Khung RTR & Đồng bộ 2 Node

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (CO3037 / CO3038)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `lab3_canbus` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Mục tiêu thí nghiệm](#1-mục-tiêu-thí-nghiệm)
- [2. Cấu trúc thư mục dự án](#2-cấu-trúc-thư-mục-dự-án)
- [3. Phân bố chân phần cứng & Sơ đồ nối dây Bus CAN](#3-phân-bố-chân-phần-cứng--sơ-đồ-nối-dây-bus-can)
  - [3.1. Cấu hình chân CAN1 trên STM32F407](#31-cấu-hình-chân-can1-trên-stm32f407)
  - [3.2. Sơ đồ mạch kết nối vật lý giữa 2 Kit BKIT ARM4](#32-sơ-đồ-mạch-kết-nối-vật-lý-giữa-2-kit-bkit-arm4)
- [4. Danh mục các bài thực hành chi tiết](#4-danh-mục-các-bài-thực-hành-chi-tiết)
  - [4.1. Các bài Demo nền tảng (demos/)](#41-các-bài-demo-nền-tảng-demos)
  - [4.2. Các bài tập ứng dụng thực hành (lab3_ex*)](#42-các-bài-tập-ứng-dụng-thực-hành-lab3_ex)
    - [Bài 1: lab3_ex1_can_sensor_loopback - Đóng gói & Giải mã cảm biến](#bài-1-lab3_ex1_can_sensor_loopback---đóng-gói--giải-mã-cảm-biến)
    - [Bài 2: lab3_ex2_can_command_rtr - Khung yêu cầu từ xa RTR](#bài-2-lab3_ex2_can_command_rtr---khung-yêu-cầu-từ-xa-rtr)
    - [Bài 3: lab3_ex3_can_filter_priority - Phân xử ưu tiên & Bộ lọc Mask](#bài-3-lab3_ex3_can_filter_priority---phân-xử-ưu-tiên--bộ-lọc-mask)
    - [Bài 4: lab3_ex4_can_heartbeat_ping - Giám sát sống còn Ping-Pong](#bài-4-lab3_ex4_can_heartbeat_ping---giám-sát-sống-còn-ping-pong)
    - [Bài 5: lab3_ex5_distributed_traffic_light - Đèn giao thông phân tán](#bài-5-lab3_ex5_distributed_traffic_light---đèn-giao-thông-phân-tán)
- [5. Hướng dẫn biên dịch & Nạp chương trình](#5-hướng-dẫn-biên-dịch--nạp-chương-trình)
- [6. Quy tắc kỹ thuật & Lưu ý cốt lõi](#6-quy-tắc-kỹ-thuật--lưu-ý-cốt-lõi)

---

## 🎯 1. Mục tiêu thí nghiệm

1. **Làm chủ chuẩn truyền thông công nghiệp CAN Bus (Controller Area Network - ISO 11898):**
   - Cơ chế phân xử đường truyền không phá hủy (*Non-destructive Bitwise Arbitration*) dựa trên Identifier (ID nhỏ hơn có độ ưu tiên cao hơn).
   - Nắm vững cấu trúc các loại khung CAN: Khung dữ liệu (*Data Frame*), Khung yêu cầu truyền từ xa (*Remote Transmission Request - RTR Frame*), Khung báo lỗi (*Error Frame*).
2. **Khai thác bộ điều khiển phần cứng STM32 bxCAN trong Zephyr RTOS:**
   - Cấu hình Bit Timing (BRP, Phase Segment 1, Phase Segment 2, SJW) đạt tốc độ chuẩn công nghiệp: **250 kbps** hoặc **500 kbps**.
   - Thiết lập bộ lọc phần cứng (*Hardware Acceptance Filters & Masks*) qua API `can_add_rx_filter()` nhằm loại bỏ các bản tin rác ngay tại phần cứng, giải phóng hoàn toàn CPU.
3. **Chuyển đổi linh hoạt chế độ kiểm thử:**
   - Chế độ nội bộ `CAN_MODE_LOOPBACK`: Tự kiểm thử stack mạng trên 1 kit duy nhất.
   - Chế độ mạng vi sai thực tế `CAN_MODE_NORMAL`: Kết nối liên trạm qua bộ thu phát CAN Transceiver (TJA1050 / SN65HVD230).
4. **Xây dựng hệ thống nhúng phân tán (Distributed Embedded Systems):** Hiện thực giao thức giám sát sống còn (*Heartbeat*) và điều khiển cụm đèn giao thông đồng bộ giữa 2 kit Master - Slave.

---

## 📁 2. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab3_canbus)
├── demos/                              # Các bài demo khảo sát chuẩn CAN Bus
│   ├── lab3_demo1_loopback/            # Demo 1: Kiểm thử vòng lặp Loopback Mode
│   ├── lab3_demo2_filters/             # Demo 2: Cấu hình bộ lọc Mask phần cứng
│   └── lab3_demo3_two_nodes/           # Demo 3: Giao tiếp thực tế giữa 2 Kit Master-Slave
├── lab3_ex1_can_sensor_loopback/       # Bài 1: Đóng gói và giải mã dữ liệu cảm biến đa byte
├── lab3_ex2_can_command_rtr/           # Bài 2: Điều khiển thiết bị và hỏi đáp qua khung RTR
├── lab3_ex3_can_filter_priority/       # Bài 3: Kiểm chứng cơ chế phân xử ưu tiên không phá hủy
├── lab3_ex4_can_heartbeat_ping/        # Bài 4: Giám sát kết nối sống còn Ping-Pong 2 Node
├── lab3_ex5_distributed_traffic_light/ # Bài 5: Đèn giao thông phân tán Master - Slave qua CAN
├── .gitignore
└── README.md                           # Tài liệu kỹ thuật Lab 3
```

---

## 🔌 3. Phân bố chân phần cứng & Sơ đồ nối dây Bus CAN

### 3.1. Cấu hình chân CAN1 trên STM32F407
Tập tin Devicetree overlay (`app.overlay`) cấu hình ngoại vi bxCAN và các chân điều khiển:

| Tín hiệu ngoại vi | Chân GPIO STM32F407 | Chức năng Alternate Function | Trạng thái kỹ thuật |
| :--- | :--- | :--- | :--- |
| **CAN1_RX** | `PB8` | AF9 (CAN1) | Nhận dữ liệu từ CAN Transceiver |
| **CAN1_TX** | `PB9` | AF9 (CAN1) | Xuất dữ liệu tới CAN Transceiver |
| **LED3 / Output Y0 / Y1** | `PE4` / `PE5` / `PE6` | GPIO Output | Chỉ thị trạng thái và tín hiệu đèn |
| **Console UART** | `PA9` (TX) / `PA10` (RX) | `usart1` | 115200 bps, 8-N-1 (Serial Log) |

---

### 3.2. Sơ đồ mạch kết nối vật lý giữa 2 Kit BKIT ARM4
Khi vận hành mạng 2 trạm (Bài 4 và Bài 5), kết nối qua bộ chuyển đổi thu phát CAN (Transceiver):

```text
       KIT A (Master / Node 1)                 KIT B (Slave / Node 2)
  +-------------------------------+       +-------------------------------+
  |  STM32F407      CAN Trcv      |       |  STM32F407      CAN Trcv      |
  |  PB9 (TX)  ---> TXD           |       |  PB9 (TX)  ---> TXD           |
  |  PB8 (RX)  <--- RXD           |       |  PB8 (RX)  <--- RXD           |
  |                               |       |                               |
  |             [CAN_H] ----------+-------+---------- [CAN_H]             |
  |                |              |       |              |                |
  |              [120Ω]           |  CAN  |            [120Ω]             |
  |                |              |  BUS  |              |                |
  |             [CAN_L] ----------+-------+---------- [CAN_L]             |
  |                               |       |                               |
  |             [GND] ------------+-------+---------- [GND]               |
  +-------------------------------+       +-------------------------------+
```

> **Yêu cầu đấu nối bắt buộc:**
> 1. **Trở đầu cuối 120Ω (Terminating Resistors):** Bắt buộc phải gắn điện trở 120 Ohm ở cả 2 đầu tuyến bus để triệt tiêu sóng phản xạ đường truyền vi sai.
> 2. **Nối chung GND:** Đường nối đất tham chiếu chung giữa 2 board là bắt buộc để duy trì mức điện áp chế độ chung (*Common-mode Voltage*) ổn định cho bộ thu phát.

---

## 📊 4. Danh mục các bài thực hành chi tiết

### 4.1. Các bài Demo nền tảng (`demos/`)

- **`lab3_demo1_loopback` (Self-Testing):** Hoạt động ở chế độ `CAN_MODE_LOOPBACK`. Node tự đóng gói khung tin chuẩn (ID `0x123`, 4 bytes dữ liệu), tự truyền và thu lại vào bộ đệm RX nhằm kiểm tra tính tương thích của driver phần cứng trong Zephyr RTOS.
- **`lab3_demo2_filters` (Hardware Filtering):** Cấu hình 2 bộ lọc phần cứng riêng biệt:
  - *Bộ lọc 1:* Bắt chính xác ID `0x100` với Mask tuyệt đối `0x7FF`.
  - *Bộ lọc 2:* Bắt dải ID từ `0x200` đến `0x20F` với Mask `0x7F0`.
  - Chứng minh các gói tin có ID nằm ngoài dải lọc sẽ bị phần cứng CAN hủy bỏ tức thì, không gây ngắt CPU.
- **`lab3_demo3_two_nodes` (Dual-Node Bus):** Kết nối 2 kit thực tế ở chế độ `CAN_MODE_NORMAL`. Node 1 và Node 2 định kỳ trao đổi khung Heartbeat (`0x701` và `0x702`), đồng thời theo dõi trạng thái bus qua hàm `can_get_state()`.

---

### 4.2. Các bài tập ứng dụng thực hành (`lab3_ex*`)

#### 📦 Bài 1: `lab3_ex1_can_sensor_loopback` - Đóng gói & Giải mã cảm biến
- **Mục tiêu:** Kỹ thuật đóng gói cấu trúc dữ liệu đa tham số môi trường vào khung truyền 8 bytes chuẩn CAN.
- **Quy chuẩn Payload (ID: `0x200`, DLC: 8 bytes, Little-Endian):**
  - `Byte 0-1`: Nhiệt độ môi trường (`int16_t`, hệ số tỉ lệ $0.1^\circ\text{C}$).
  - `Byte 2`: Độ ẩm tương đối (`uint8_t`, đơn vị %).
  - `Byte 3-4`: Áp suất khí quyển (`uint16_t`, đơn vị hPa).
  - `Byte 5`: Mã trạng thái hệ thống (`uint8_t`).
  - `Byte 6-7`: Giá trị Checksum / Bộ đếm khung tự tăng (`uint16_t`).
- **Kiểm chứng:** Giải mã chính xác số liệu tại callback ngắt nhận và in định dạng bảng ra UART console.

---

#### 🔄 Bài 2: `lab3_ex2_can_command_rtr` - Khung yêu cầu từ xa RTR
- **Mục tiêu:** Vận hành cơ chế hỏi - đáp trạng thái từ xa không tốn băng thông payload.
- **Thiết kế:**
  - **Node Master:** Phát khung RTR Frame (`flags = CAN_FRAME_RTR`, `dlc = 0`, ID `0x301`).
  - **Node Slave:** Cấu hình bộ lọc phần cứng bắt RTR trên ID `0x301`. Khi phát hiện yêu cầu, Slave tự động bắn ngược lại Data Frame chứa trạng thái hiện tại của Rơ-le và các ngõ ra Y0/Y1.
  - Đồng thời Master có thể phát lệnh Data Frame mang ID `0x300` để điều khiển bật/tắt thiết bị tức thì.

---

#### ⚖️ Bài 3: `lab3_ex3_can_filter_priority` - Phân xử ưu tiên & Bộ lọc Mask
- **Mục tiêu:** Thực nghiệm cơ chế phân xử đường truyền không phá hủy (*Non-destructive Arbitration*).
- **Thực thi:**
  - Hai luồng phát đồng thời 2 khung CAN tại cùng một thời điểm:
    - *Khung Khẩn cấp (Alarm / Emergency):* ID **`0x080`** (Ưu tiên cao hơn vì ID nhỏ hơn).
    - *Khung Định kỳ (Telemetry):* ID **`0x400`** (Ưu tiên thấp hơn).
  - **Kết quả:** Khung `0x080` luôn giành quyền bus và truyền hoàn tất trước, khung `0x400` tự động hoãn lại và phát ngay sau đó mà không làm hỏng dữ liệu của nhau.

---

#### 💓 Bài 4: `lab3_ex4_can_heartbeat_ping` - Giám sát sống còn Ping-Pong
- **Mục tiêu:** Thiết lập cơ chế giám sát kết nối mạng vật lý phân tán giữa 2 vi điều khiển.
- **Thực thi:**
  - **Kit A (Node 1):** Định kỳ mỗi 1000 ms phát gói tin Heartbeat mang ID `0x701`.
  - **Kit B (Node 2):** Khởi tạo bộ đếm thời gian giám sát 2500 ms. Mỗi lần nhận được gói Heartbeat từ Kit A, timer được reset. Nếu mất kết nối quá 2.5 giây, Kit B lập tức kích hoạt đèn cảnh báo sự cố `[NODE 1 DISCONNECTED]`.
  - Hỗ trợ giao thức Ping-Pong đo đạc độ trễ khứ hồi (*Round-Trip Time - RTT*).

---

#### 🚦 Bài 5: `lab3_ex5_distributed_traffic_light` - Đèn giao thông phân tán
- **Mục tiêu:** Xây dựng hệ sinh thái đèn giao thông thông minh hoạt động phân tán qua mạng CAN Bus.
- **Thực thi:**
  - **Kit A (Controller Master):** Chạy FSM đèn giao thông (Đỏ 7s, Xanh 5s, Vàng 2s), tính toán thời gian đếm ngược mỗi giây và phát broadcast lên bus với ID `0x250`: `{ uint8_t phase_id; uint8_t remaining_sec; }`.
  - **Kit B (Display Slave):** Đặt tại hiện trường, đón nhận ID `0x250` để đồng bộ hiển thị đèn LED và in số giây đếm ngược ra màn hình.
  - **Cơ chế Bảo an (Fail-Safe):** Nếu đường truyền CAN bị đứt quá 3 giây, Kit B tự động ngắt kết nối và chuyển sang chế độ nháy Đèn Vàng khẩn cấp.

---

## 💻 5. Hướng dẫn biên dịch & Nạp chương trình

Thực hiện lệnh với công cụ `west` từ thư mục gốc nhánh `lab3_canbus`:

```bash
# 1. Biên dịch một bài tập cụ thể (Ví dụ: Bài 5 Đèn giao thông phân tán)
west build -p always -b stm32f4_disco lab3_ex5_distributed_traffic_light

# 2. Nạp firmware vào Kit BKIT ARM4 qua mạch nạp ST-Link
west flash

# 3. Theo dõi nhật ký truyền nhận qua cổng Serial UART (115200 bps, 8-N-1)
picocom -b 115200 /dev/ttyUSB0
```

---

## ⚙️ 6. Quy tắc kỹ thuật & Lưu ý cốt lõi

1. **Quy tắc phân định Identifier:** Trong chuẩn CAN Bus, bit mang mức logic `0` là bit trội (*Dominant bit*), bit mang mức `1` là bit lặn (*Recessive bit*). Do đó, thông điệp mang ID có giá trị số nhỏ hơn sẽ luôn giành chiến thắng trong pha phân xử bus.
2. **Khai báo bộ lọc hiệu quả:** Hàm `can_add_rx_filter()` trả về một `filter_id`. Khi không còn sử dụng, phải gọi `can_remove_rx_filter()` để tránh lãng phí thanh ghi lọc phần cứng của bộ điều khiển bxCAN (vốn có giới hạn số lượng bộ lọc).
3. **Quản trị tải đường truyền (Bus Load):** Trong mạng CAN, không nên để tải đường truyền vượt quá 70% băng thông thiết kế nhằm phòng ngừa tình trạng chậm trễ (*Jitter*) đối với các khung tin định kỳ.
