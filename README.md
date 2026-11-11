# 🚗 LAB 3: MẠNG TRUYỀN THÔNG CAN BUS VÀ HỆ THỐNG PHÂN TÁN TRONG ZEPHYR RTOS
### Lý thuyết chuyên sâu về Giao thức CAN, Bộ điều khiển bxCAN, Định thời bit, Bộ lọc ID/Mask & Lập trình ứng dụng

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (Embedded Systems)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `lab3_canbus` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6 + Transceiver TJA1050)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Mục tiêu thí nghiệm](#1-mục-tiêu-thí-nghiệm)
- [2. Kiến thức nền tảng & Lý thuyết chuyên sâu về CAN Bus](#2-kiến-thức-nền-tảng--lý-thuyết-chuyên-sâu-về-can-bus)
  - [2.1. Giới thiệu tổng quan về mạng CAN Bus](#21-giới-thiệu-tổng-quan-về-mạng-can-bus)
  - [2.2. Tầng vật lý: Tín hiệu vi sai và Mức logic Dominant / Recessive](#22-tầng-vật-lý-tín-hiệu-vi-sai-và-mức-logic-dominant--recessive)
  - [2.3. Điện trở kết cuối Bus (Termination Resistor 120 Ohm)](#23-điện-trở-kết-cuối-bus-termination-resistor-120-ohm)
  - [2.4. Cấu trúc khung truyền CAN (CAN Frame Architecture)](#24-cấu-trúc-khung-truyền-can-can-frame-architecture)
  - [2.5. Cơ chế phân xử bus không phá hủy theo ID (Non-destructive Bitwise Arbitration)](#25-cơ-chế-phân-xử-bus-không-phá-hủy-theo-id-non-destructive-bitwise-arbitration)
  - [2.6. Định thời bit và Lấy mẫu dữ liệu (Bit Timing & Sample Point)](#26-định-thời-bit-và-lấy-mẫu-dữ-liệu-bit-timing--sample-point)
  - [2.7. Quản lý lỗi và Các trạng thái của Node (Error Management)](#27-quản-lý-lỗi-và-các-trạng-thái-của-node-error-management)
  - [2.8. Bộ lọc nhận phần cứng bxCAN (Hardware Acceptance Filtering ID/Mask)](#28-bộ-lọc-nhận-phần-cứng-bxcan-hardware-acceptance-filtering-idmask)
  - [2.9. API CAN của Zephyr RTOS đối chiếu với STM32 HAL (CubeIDE)](#29-api-can-của-zephyr-rtos-đối-chiếu-với-stm32-hal-cubeide)
- [3. Cấu hình phần cứng trên Kit BKIT ARM4](#3-cấu-hình-phần-cứng-trên-kit-bkit-arm4)
- [4. Cấu trúc thư mục dự án](#4-cấu-trúc-thư-mục-dự-án)
- [5. Danh mục 3 bài Demo nền tảng (demos/)](#5-danh-mục-3-bài-demo-nền-tảng-demos)
  - [Demo 1: lab3_demo1_loopback - Gửi nhận CAN chế độ Loopback nội bộ](#demo-1-lab3_demo1_loopback---gửi-nhận-can-chế-độ-loopback-nội-bộ)
  - [Demo 2: lab3_demo2_filters - Kiểm nghiệm bộ lọc phần cứng ID/Mask và RTR](#demo-2-lab3_demo2_filters---kiểm-nghiệm-bộ-lọc-phần-cứng-idmask-và-rtr)
  - [Demo 3: lab3_demo3_two_nodes - Giao tiếp giữa 2 Node trên Bus thật & Giám sát lỗi](#demo-3-lab3_demo3_two_nodes---giao-tiếp-giữa-2-node-trên-bus-thật--giám-sát-lỗi)
- [6. Danh mục 5 bài tập thực hành ứng dụng (lab3_ex*)](#6-danh-mục-5-bài-tập-thực-hành-ứng-dụng-lab3_ex)
  - [Bài 1: lab3_ex1_can_sensor_loopback - Đóng gói dữ liệu cảm biến đa byte Little-Endian](#bài-1-lab3_ex1_can_sensor_loopback---đóng-gói-dữ-liệu-cảm-biến-đa-byte-little-endian)
  - [Bài 2: lab3_ex2_can_command_rtr - Khung yêu cầu từ xa RTR và Điều khiển thiết bị](#bài-2-lab3_ex2_can_command_rtr---khung-yêu-cầu-từ-xa-rtr-và-điều-khiển-thiết-bị)
  - [Bài 3: lab3_ex3_can_filter_priority - Phân xử ưu tiên bus giữa Khẩn cấp và Định kỳ](#bài-3-lab3_ex3_can_filter_priority---phân-xử-ưu-tiên-bus-giữa-khẩn-cấp-và-định-kỳ)
  - [Bài 4: lab3_ex4_can_heartbeat_ping - Giám sát sống còn Node và Giao thức Ping-Pong](#bài-4-lab3_ex4_can_heartbeat_ping---giám-sát-sống-còn-node-và-giao-thức-ping-pong)
  - [Bài 5: lab3_ex5_distributed_traffic_light - Đèn giao thông phân tán Master - Slave](#bài-5-lab3_ex5_distributed_traffic_light---đèn-giao-thông-phân-tán-master---slave)
- [7. Hướng dẫn biên dịch, Nạp chương trình & Đấu dây thực nghiệm](#7-hướng-dẫn-biên-dịch-nạp-chương-trình--đấu-dây-thực-nghiệm)
- [8. Tổng kết các lỗi kỹ thuật thường gặp & Bảng Cheat-sheet](#8-tổng-kết-các-lỗi-kỹ-thuật-thường-gặp--bảng-cheat-sheet)

---

## 🎯 1. Mục tiêu thí nghiệm

1. **Hiểu thấu đáo bản chất mạng CAN (Controller Area Network):** Nắm vững kiến trúc tầng vật lý vi sai, cơ chế đồng bộ bit, và giải thuật phân xử đường truyền không phá hủy (*Non-destructive Bitwise Arbitration*).
2. **Cấu hình phần cứng bộ điều khiển CAN trong Zephyr:** Khai báo kích hoạt ngoại vi `can1` trong Devicetree (`app.overlay`), ánh xạ chân `PB8 (RX)` và `PB9 (TX)`, thiết lập bitrate chuẩn $500\text{ kbit/s}$ qua Kconfig.
3. **Thành thạo API CAN của Zephyr RTOS:**
   - Chuyển đổi linh hoạt giữa các chế độ: `CAN_MODE_LOOPBACK`, `CAN_MODE_NORMAL`, `CAN_MODE_LISTENONLY`.
   - Đóng gói và gửi khung tin (`struct can_frame`): Standard 11-bit, Extended 29-bit, RTR frame.
   - Sử dụng bộ lọc phần cứng kết hợp Message Queue: `CAN_MSGQ_DEFINE` và `can_add_rx_filter_msgq()`.
4. **Xây dựng hệ thống phân tán thực tế:** Kết nối 2 Kit BKIT ARM4 qua đường truyền vật lý, thiết lập cơ chế giám sát lỗi bus (*Bus-off, Error-Passive*) và xây dựng hệ thống đèn giao thông phân tán có cơ chế bảo an (*Fail-Safe*).

---

## 📚 2. Kiến thức nền tảng & Lý thuyết chuyên sâu về CAN Bus

### 2.1. Giới thiệu tổng quan về mạng CAN Bus
**CAN (Controller Area Network)** là chuẩn mạng truyền thông nối tiếp bus đa chủ (Multi-master), được phát triển bởi Robert Bosch GmbH vào những năm 1980 và chuẩn hóa theo tiêu chuẩn quốc tế **ISO 11898**.
* **Đặc tính Message-based:** Không node nào có địa chỉ mạng cố định (như IP hay MAC). Thông điệp được định danh bởi một **Identifier (ID)** duy nhất phản ánh nội dung dữ liệu (ví dụ: Tốc độ xe, Nhiệt độ động cơ).
* **Broadcast:** Mọi node trên bus đều nhận được mọi thông điệp và tự quyết định xem có xử lý hay bỏ qua thông qua bộ lọc phần cứng (*Acceptance Filter*).

---

### 2.2. Tầng vật lý: Tín hiệu vi sai và Mức logic Dominant / Recessive

Mạng CAN sử dụng hai dây dẫn xoắn đôi: **CAN_H (CAN High)** và **CAN_L (CAN Low)** để truyền tín hiệu vi sai:
$$V_{diff} = V_{CAN\_H} - V_{CAN\_L}$$

```
    Điện áp (V)
       3.5 V ──┐                           ┌──  CAN_H (3.5 V)
               │                           │
       2.5 V ──┼───────────────────────────┼──  Trạng thái nghỉ Recessive (V_diff ≈ 0 V)
               │                           │
       1.5 V ──┘                           └──  CAN_L (1.5 V)
               ◄── Bit Dominant (0) ──────► ◄── Bit Recessive (1) ──►
               (V_diff = 3.5 - 1.5 = 2.0 V)   (V_diff = 2.5 - 2.5 = 0 V)
```

#### Hai mức logic cơ bản:
1. **Bit Trội (Dominant bit - Mức logic `0`):** Transceiver kích hoạt đẩy $V_{CAN\_H} \approx 3.5\text{V}$ và kéo $V_{CAN\_L} \approx 1.5\text{V} \implies V_{diff} \approx 2.0\text{V}$.
2. **Bit Lặn (Recessive bit - Mức logic `1`):** Transceiver ở trạng thái trở kháng cao (High-Z), hai dây được giữ ở mức tĩnh $V_{CAN\_H} \approx V_{CAN\_L} \approx 2.5\text{V} \implies V_{diff} \approx 0\text{V}$.

> [!IMPORTANT]
> **Quy tắc "AND nối dây" (Wired-AND Logic):**
> Do cấu tạo tầng output của Transceiver (như TJA1050), chỉ cần **MỘT node duy nhất** phát bit Dominant (`0`), toàn bộ đường truyền bus sẽ bị kéo xuống mức Dominant (`0`), đè bẹp bất kỳ node nào đang phát bit Recessive (`1`).

---

### 2.3. Điện trở kết cuối Bus (Termination Resistor 120 Ohm)
Theo tiêu chuẩn ISO 11898-2 (High-Speed CAN):
- Tốc độ truyền cao (lên tới $1\text{ Mbit/s}$) khiến tín hiệu điện có tính chất sóng lan truyền trên đường dây dài. Nếu đầu cuối hở mạch, sóng điện sẽ bị dội ngược trở lại (*Signal Reflection*) gây nhiễu và làm sai lệch bit.
- Bắt buộc phải gắn **hai điện trở $120\,\Omega$** tại hai điểm mút xa nhất của mạng bus.
- Khi đo trở kháng giữa CAN_H và CAN_L lúc ngắt nguồn toàn hệ thống:
  $$R_{bus} = 120\,\Omega \parallel 120\,\Omega = 60\,\Omega$$
- **Trên Kit BKIT ARM4:** Đã tích hợp sẵn điện trở $120\,\Omega$ đi kèm công tắc gạt **SW13**. Nếu kit nằm ở đầu mút bus, gạt SW13 sang **ON**; nếu kit nằm ở giữa bus, gạt sang **OFF**.

---

### 2.4. Cấu trúc khung truyền CAN (CAN Frame Architecture)

#### Khung dữ liệu chuẩn (Standard Data Frame - CAN 2.0A):
```
┌─────┬────────────────┬───────┬────────────┬─────────────┬───────────┬─────────┬─────┐
│ SOF │ Identifier 11b │ RTR 1b│ Control 6b │ Data 0..8 B │  CRC 16b  │ ACK 2b  │ EOF │
└─────┴────────────────┴───────┴────────────┴─────────────┴───────────┴─────────┴─────┘
```
1. **SOF (Start of Frame - 1 bit Dominant):** Đồng bộ hóa bộ đếm thời gian của các node khi bus đang rảnh.
2. **Arbitration Field (Trường phân xử):**
   * **Identifier (11 bits hoặc 29 bits):** Độ ưu tiên và phân loại thông điệp.
   * **RTR (Remote Transmission Request - 1 bit):** Mức `0` đối với Data Frame; mức `1` đối với Remote Frame.
3. **Control Field (Trường điều khiển - 6 bits):** Gồm bit IDE (nhận biết ID 11-bit hay 29-bit), r0 (dự phòng) và **DLC (Data Length Code - 4 bits)** quy định số byte dữ liệu từ $0$ đến $8$.
4. **Data Field (Trường dữ liệu):** Chứa từ $0$ đến $8$ bytes dữ liệu thực tế.
5. **CRC Field (Kiểm tra lỗi):** 15 bits mã kiểm tra đa thức CRC + 1 bit CRC Delimiter (Recessive).
6. **ACK Field (Xác nhận - 2 bits):**
   * *ACK Slot:* Node phát gửi bit Recessive (`1`). Bất kỳ node nào trên bus nhận đúng CRC đều kéo chân bus xuống Dominant (`0`) để báo nhận thành công.
   * *ACK Delimiter:* 1 bit Recessive.
7. **EOF (End of Frame - 7 bits Recessive):** Đánh dấu kết thúc khung tin.

---

### 2.5. Cơ chế phân xử bus không phá hủy theo ID (Non-destructive Bitwise Arbitration)

Khi hai hoặc nhiều node cùng bắt đầu truyền dữ liệu tại cùng một thời điểm:
1. Các node truyền đồng thời từng bit của trường Identifier ra bus, và **đồng thời đọc ngược lại mức logic thực tế trên bus**.
2. Khi Node A phát bit `0` (Dominant) và Node B phát bit `1` (Recessive):
   * Mức thực tế trên bus sẽ là `0` (do tính chất Dominant đè Recessive).
   * Node A đọc thấy `0` (trùng khớp với bit mình vừa phát) $\to$ tiếp tục truyền bit tiếp theo.
   * Node B đọc thấy `0` (khác với bit `1` mình vừa phát) $\to$ nhận ra mình đã thua cuộc phân xử!
   * Node B **lập tức chuyển sang chế độ nhận (Receiver)** mà không phát tiếp, không làm hỏng bit đang phát của Node A.

> [!IMPORTANT]
> **Ý nghĩa thực tế:**
> Node nào mang **Identifier có giá trị số nhỏ hơn** sẽ phát ra bit `0` sớm hơn $\implies$ **Giành quyền ưu tiên cao hơn trên bus CAN**. Khung tin ưu tiên cao nhất sẽ truyền thông suốt mà không chịu bất kỳ độ trễ nào!

---

### 2.6. Định thời bit và Lấy mẫu dữ liệu (Bit Timing & Sample Point)

Mỗi bit trên đường truyền CAN được chia thành các khoảng thời gian nhỏ gọi là **Time Quantum ($t_q$):**

```
┌──────────┬──────────────┬──────────────┬──────────────┐
│ SYNC_SEG │   PROP_SEG   │  PHASE_SEG1  │  PHASE_SEG2  │
└──────────┴──────────────┴──────────────┴──────────────┘
           ◄──────── Time Segment 1 ─────►◄── Seg 2 ────►
                                         ▲
                                    Sample Point (Lấy mẫu tại 75% - 87.5%)
```
1. **SYNC_SEG ($1\,t_q$):** Đồng bộ sườn xung.
2. **PROP_SEG ($1..8\,t_q$):** Bù trễ vật lý lan truyền trên đường dây và qua Transceiver.
3. **PHASE_SEG1 ($1..8\,t_q$):** Bù trễ pha trước điểm lấy mẫu.
4. **PHASE_SEG2 ($1..8\,t_q$):** Khoảng thời gian sau điểm lấy mẫu để hoàn tất bit.
- **Tần số bus chuẩn:** Bộ điều khiển `bxCAN` trên STM32F4 kết nối với bus ngoại vi APB1 ($42\text{ MHz}$). Với bitrate $500\text{ kbit/s}$, mỗi bit kéo dài $2000\text{ ns}$, tương ứng với cấu hình $14\,t_q$ ($1\,t_q = 142.85\text{ ns}$, Sample Point tại $85.7\%$).

---

### 2.7. Quản lý lỗi và Các trạng thái của Node (Error Management)

Mỗi bộ điều khiển CAN tích hợp 2 bộ đếm lỗi phần cứng:
- **TEC (Transmit Error Counter):** Bộ đếm lỗi khi truyền.
- **REC (Receive Error Counter):** Bộ đếm lỗi khi nhận.

```
       TEC/REC < 128                  TEC/REC ≥ 128                 TEC > 255
   ┌───────────────────┐          ┌───────────────────┐          ┌───────────────────┐
   │   ERROR-ACTIVE    │ ───────► │   ERROR-PASSIVE   │ ───────► │      BUS-OFF      │
   │ (Hoạt động chuẩn, │ ◄─────── │  (Cảnh báo lỗi,   │          │ (Bị cách ly khỏi  │
   │  phát Active Flag)│          │ phát Passive Flag)│          │  bus hoàn toàn)   │
   └───────────────────┘          └───────────────────┘          └───────────────────┘
```
* **Chế độ tự phục hồi (Auto-Recovery):** Trong Zephyr RTOS, driver STM32 mặc định kích hoạt tự phục hồi khi thoát khỏi Bus-off sau 128 lần xuất hiện 11 bit recessive liên tiếp trên bus.

---

### 2.8. Bộ lọc nhận phần cứng bxCAN (Hardware Acceptance Filtering ID/Mask)

Bộ điều khiển CAN của vi điều khiển lọc frame bằng thanh ghi phần cứng trước khi ngắt CPU:
* **Bộ lọc gồm 2 thanh ghi:** `filter_id` và `filter_mask`.
* **Quy tắc bitwise:**
  $$\text{Bit kiểm tra} = (\text{Received\_ID} \oplus \text{Filter\_ID}) \ \& \ \text{Filter\_Mask}$$
  - Nếu kết quả bằng `0`: Frame được chấp nhận và đẩy vào hàng đợi Message Queue.
  - Bit tại vị trí nào trong `Filter_Mask` bằng **`1`**: Bắt buộc bit tương ứng trong ID nhận về phải **giống hệt** với `Filter_ID`.
  - Bit tại vị trí nào trong `Filter_Mask` bằng **`0`**: Bỏ qua (**Don't care** - có thể là 0 hoặc 1 đều được chấp nhận).

---

### 2.9. API CAN của Zephyr RTOS đối chiếu với STM32 HAL (CubeIDE)

| Chức năng | Zephyr RTOS v4.4.0 API | STM32 HAL (STM32CubeIDE) |
| :--- | :--- | :--- |
| **Lấy thiết bị** | `DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))` | Con trỏ cấu trúc `&hcan1` |
| **Cấu hình chế độ** | `can_set_mode(dev, CAN_MODE_LOOPBACK)` | `hcan1.Init.Mode = CAN_MODE_LOOPBACK;` |
| **Khởi động CAN** | `can_start(dev)` | `HAL_CAN_Start(&hcan1);` |
| **Gửi frame (Chặn)**| `can_send(dev, &frame, K_FOREVER, NULL, NULL)`| `HAL_CAN_AddTxMessage(&hcan1, ...)` |
| **Gửi frame (Non-block)**| `can_send(dev, &frame, K_MSEC(100), cb, data)`| `HAL_CAN_AddTxMessage` kèm callback |
| **Thêm bộ lọc Queue**| `can_add_rx_filter_msgq(dev, &q, &filter)`| `HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig)`|
| **Giám sát trạng thái**| `can_set_state_change_callback(dev, cb, ...)`| `HAL_CAN_ErrorCallback(&hcan1)` |

---

## 🔌 3. Cấu hình phần cứng trên Kit BKIT ARM4

### 3.1. Sơ đồ kết nối phần cứng
Trên Kit BKIT ARM4:
- Bộ điều khiển **bxCAN1** của STM32F407 được nối với chip Transceiver **TJA1050**:
  - `PB8` $\rightarrow$ `CAN1_RX`
  - `PB9` $\rightarrow$ `CAN1_TX`
- Transceiver chuyển đổi tín hiệu ra cầu đấu vít: **CAN_H** và **CAN_L**.
- Công tắc gạt **SW13**: Đóng/cắt điện trở kết cuối $120\,\Omega$.

### 3.2. Cấu hình Devicetree (`app.overlay`)
```dts
/ {
    chosen {
        zephyr,canbus = &can1;
    };
};

&can1 {
    status = "okay";
    pinctrl-0 = <&can1_rx_pb8 &can1_tx_pb9>;
    pinctrl-names = "default";
    bus-speed = <500000>; /* Bitrate 500 kbit/s */
};
```

---

## 📁 4. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab3_canbus)
├── demos/                              # Thư mục chứa 3 bài Demo khảo sát nguyên lý CAN
│   ├── lab3_demo1_loopback/            # Demo 1: Khởi tạo và kiểm tra CAN chế độ Loopback
│   ├── lab3_demo2_filters/             # Demo 2: Khảo sát bộ lọc ID/Mask phần cứng và RTR
│   └── lab3_demo3_two_nodes/           # Demo 3: Nối 2 Node trên bus thật và giám sát Bus-off
├── lab3_ex1_can_sensor_loopback/       # Bài tập 1: Đóng gói và giải mã dữ liệu cảm biến
├── lab3_ex2_can_command_rtr/           # Bài tập 2: Giao thức hỏi đáp RTR và điều khiển thiết bị
├── lab3_ex3_can_filter_priority/       # Bài tập 3: Phân xử độ ưu tiên bus khẩn cấp vs định kỳ
├── lab3_ex4_can_heartbeat_ping/        # Bài tập 4: Giám sát sống còn Node và đo trễ Ping-Pong
├── lab3_ex5_distributed_traffic_light/ # Bài tập 5: Đèn giao thông phân tán Master - Slave
├── LAB3_MANUAL_Zephyr_BKIT_ARM4.docx   # File tài liệu hướng dẫn thực hành gốc Lab 3
├── LAB3_EXERCISE_Zephyr_BKIT_ARM4.docx # File phiếu bài tập và mẫu nộp bài Lab 3
├── .gitignore
└── README.md                           # Tài liệu kỹ thuật chi tiết Lab 3
```

---

## 🔬 5. Danh mục 3 bài Demo nền tảng (`demos/`)

### Demo 1: `lab3_demo1_loopback` - Gửi nhận CAN chế độ Loopback nội bộ
* **Mục tiêu:** Làm quen trình tự khởi tạo driver CAN (`can_set_mode` $\rightarrow$ `can_add_rx_filter_msgq` $\rightarrow$ `can_start`), kiểm tra logic truyền nhận mà không cần dây nối bên ngoài.
* **Cơ chế:** Ở chế độ `CAN_MODE_LOOPBACK`, khối logic truyền của bxCAN tự động nối ngầm vào khối nhận bên trong vi điều khiển.
* **Kết quả:** Gửi frame `ID = 0x123`, `DLC = 4`, dữ liệu `{0x11, 0x22, 0x33, 0x44}` và nhận lại chính xác qua `rx_msgq`.

---

### Demo 2: `lab3_demo2_filters` - Kiểm nghiệm bộ lọc phần cứng ID/Mask và RTR
* **Mục tiêu:** Chứng minh bộ lọc phần cứng bxCAN chỉ cho phép các frame đúng điều kiện đi vào RAM.
* **Thực nghiệm 2 bộ lọc:**
  - *Bộ lọc A (Standard ID):* `id = 0x100`, `mask = 0x7F8` (chấp nhận dải ID từ `0x100` đến `0x107`).
  - *Bộ lọc B (Extended ID):* `id = 0x18FF0001`, `mask = CAN_EXT_ID_MASK`, cờ `CAN_FILTER_IDE`.
* **Kết quả:** Chương trình gửi 9 frame thử nghiệm khác nhau, chỉ có các frame hợp lệ mới lọt qua bộ lọc vào Message Queue, các frame sai bị phần cứng loại bỏ tức thì.

---

### Demo 3: `lab3_demo3_two_nodes` - Giao tiếp giữa 2 Node trên Bus thật & Giám sát lỗi
* **Mục tiêu:** Chuyển sang `CAN_MODE_NORMAL`, kết nối 2 Kit qua dây cáp vật lý và giám sát trạng thái chuyển dịch của bộ đếm lỗi bus.
* **Cơ chế giám sát:** Đăng ký hàm callback `can_set_state_change_callback()`. Khi có sự cố đứt dây hoặc thiếu trở kết cuối, hệ thống chuyển dịch qua các trạng thái `ERROR-WARNING` $\rightarrow$ `ERROR-PASSIVE` $\rightarrow$ `BUS-OFF` và hiển thị cảnh báo lên đèn LED3.

---

## 🛠️ 6. Danh mục 5 bài tập thực hành ứng dụng (`lab3_ex*`)

### Bài 1: `lab3_ex1_can_sensor_loopback` - Đóng gói dữ liệu cảm biến đa byte Little-Endian
* **Mục tiêu:** Thiết kế giao thức đóng gói dữ liệu cảm biến công nghiệp trong 1 khung CAN 8 bytes theo chuẩn Little-Endian.
* **Định dạng Payload (ID: `0x200`, DLC: 8 bytes):**
  - `Byte 0-1`: Nhiệt độ môi trường (`int16_t`, hệ số tỉ lệ $0.1^\circ\text{C}$).
  - `Byte 2`: Độ ẩm tương đối (`uint8_t`, đơn vị %).
  - `Byte 3-4`: Áp suất khí quyển (`uint16_t`, đơn vị hPa).
  - `Byte 5`: Mã trạng thái hệ thống (`uint8_t`).
  - `Byte 6-7`: Sequence counter tự tăng (`uint16_t`).
* **Kiểm chứng:** Giải mã số học tại luồng nhận và in bảng thông số rõ ràng ra Serial Monitor.

---

### Bài 2: `lab3_ex2_can_command_rtr` - Khung yêu cầu từ xa RTR và Điều khiển thiết bị
* **Mục tiêu:** Ứng dụng khung yêu cầu từ xa RTR để đọc trạng thái cảm biến/rơ-le từ xa mà không tiêu tốn băng thông đường truyền.
* **Thực thi:**
  - **Node Master:** Phát khung RTR (`flags = CAN_FRAME_RTR`, ID `0x301`, DLC `0`).
  - **Node Slave:** Cài bộ lọc bắt RTR trên ID `0x301`. Khi phát hiện yêu cầu, Slave lập tức phản hồi Data Frame mang ID `0x301` chứa trạng thái hoạt động của đèn Y0/Y1.
  - Master có thể phát Data Frame mang ID `0x300` kèm lệnh bật/tắt để điều khiển thiết bị tức thời.

---

### Bài 3: `lab3_ex3_can_filter_priority` - Phân xử ưu tiên bus giữa Khẩn cấp và Định kỳ
* **Mục tiêu:** Thực nghiệm cơ chế phân xử đường truyền không phá hủy (*Non-destructive Arbitration*).
* **Kịch bản:**
  - Hai luồng phát đồng thời 2 khung CAN tại cùng một thời điểm:
    - *Khung Khẩn cấp (Alarm/Emergency):* ID **`0x080`** (Ưu tiên cao hơn vì ID nhỏ hơn).
    - *Khung Định kỳ (Telemetry):* ID **`0x400`** (Ưu tiên thấp hơn).
  - **Kết quả:** Khung `0x080` luôn giành chiến thắng và truyền hoàn tất trước, khung `0x400` tự động hoãn lại và phát ngay sau đó mà không hề mất mát dữ liệu.

---

### Bài 4: `lab3_ex4_can_heartbeat_ping` - Giám sát sống còn Node và Giao thức Ping-Pong
* **Mục tiêu:** Thiết lập cơ chế phát hiện đứt cáp truyền thông phân tán giữa 2 vi điều khiển.
* **Cơ chế:**
  - **Kit 1:** Định kỳ mỗi 1000 ms phát gói Heartbeat ID `0x701`.
  - **Kit 2:** Khởi tạo bộ định thời giám sát 2500 ms. Mỗi lần nhận được Heartbeat từ Kit 1, timer được nạp lại. Nếu quá 2.5 giây không nhận được tín hiệu, Kit 2 phát cảnh báo `[NODE 1 DISCONNECTED]` và nháy LED3.
  - Hỗ trợ gửi bản tin Ping và đo thời gian nhận phản hồi Pong để tính độ trễ khứ hồi (RTT).

---

### Bài 5: `lab3_ex5_distributed_traffic_light` - Đèn giao thông phân tán Master - Slave
* **Mục tiêu:** Xây dựng hệ thống đèn giao thông phân tán qua mạng bus CAN công nghiệp.
* **Kiến trúc phân tán:**
  - **Node Master (Bộ điều khiển trung tâm):** Chạy FSM đèn giao thông (Đỏ 7s, Xanh 5s, Vàng 2s). Mỗi giây phát gói tin broadcast lên bus ID `0x250`: `{ uint8_t phase_id; uint8_t remaining_seconds; }`.
  - **Node Slave (Bộ hiển thị hiện trường):** Nhận gói tin ID `0x250` để đồng bộ bật đèn tương ứng và in đếm ngược thời gian.
  - **Cơ chế Bảo an (Fail-Safe):** Nếu đường truyền CAN bị đứt hoặc mất tín hiệu quá 3 giây, Slave tự động ngắt kết nối và chuyển sang chế độ nháy Đèn Vàng khẩn cấp để đảm bảo an toàn giao thông.

---

## 💻 7. Hướng dẫn biên dịch, Nạp chương trình & Đấu dây thực nghiệm

### 7.1. Biên dịch và nạp firmware
Thực hiện lệnh với công cụ `west` từ thư mục gốc của repository:

```powershell
# 1. Kích hoạt môi trường Python ảo
cd $HOME\zephyrproject
.venv\Scripts\Activate.ps1

# 2. Biên dịch một bài tập cụ thể (Ví dụ: Bài 5 Đèn giao thông phân tán)
west build -p auto -b stm32f4_disco applications\lab3\lab3_ex5_distributed_traffic_light

# 3. Nạp firmware xuống Kit BKIT ARM4
west flash

# 4. Mở Serial Monitor theo dõi log UART (115200 baud, 8-N-1)
```

### 7.2. Hướng dẫn đấu dây khi chạy Bus thật giữa 2 Kit:
Nối dây giữa cầu đấu vít CAN của Kit A và Kit B:
* **CAN_H** của Kit A nối với **CAN_H** của Kit B.
* **CAN_L** của Kit A nối với **CAN_L** của Kit B.
* **GND** của Kit A nối với **GND** của Kit B (Bắt buộc để ổn định điện thế chung).
* Gạt công tắc **SW13** trên cả 2 kit sang vị trí **ON** để kích hoạt trở kết cuối $120\,\Omega$.

---

## ⚠️ 8. Tổng kết các lỗi kỹ thuật thường gặp & Bảng Cheat-sheet

1. **Lỗi không gửi được frame (`can_send` trả về `-EAGAIN` hoặc `-ENETUNREACH`):**
   * Chưa gọi lệnh `can_start(can_dev)`.
   * Ở chế độ bus thật: Chưa có node thứ hai ACK gói tin, hoặc chưa gạt công tắc trở kết cuối SW13.
2. **Lỗi không nhận được frame:**
   * Bộ lọc cấu hình sai `id` hoặc `mask`.
   * Quên thêm cờ `CAN_FILTER_IDE` khi lọc Identifier 29-bit mở rộng.
3. **Lỗi tràn hàng đợi Message Queue:**
   * Hàng đợi khai báo trong `CAN_MSGQ_DEFINE` có kích thước quá nhỏ (ví dụ chỉ có 2-4 phần tử), trong khi bus nhận nhiều frame liên tục. Nên cấu hình tối thiểu 8 đến 16 phần tử.
