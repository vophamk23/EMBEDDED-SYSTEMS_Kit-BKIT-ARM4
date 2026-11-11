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
- [2. Chuẩn bị phần cứng CAN trên Kit BKIT ARM4](#2-chuẩn-bị-phần-cứng-can-trên-kit-bkit-arm4)
- [3. Kiến thức nền tảng về CAN (Toàn diện theo Tài liệu hướng dẫn)](#3-kiến-thức-nền-tảng-về-can-toàn-diện-theo-tài-liệu-hướng-dẫn)
  - [3.1. Tầng vật lý: Tín hiệu vi sai](#31-tầng-vật-lý-tín-hiệu-vi-sai)
  - [3.2. Cấu trúc Frame CAN](#32-cấu-trúc-frame-can)
  - [3.3. Phân xử bus theo ID](#33-phân-xử-bus-theo-id)
  - [3.4. Định thời bit (Bit Timing)](#34-định-thời-bit-bit-timing)
  - [3.5. ACK và Xử lý lỗi Bus](#35-ack-và-xử-lý-lỗi-bus)
  - [3.6. Bộ lọc nhận phần cứng (Hardware Acceptance Filter)](#36-bộ-lọc-nhận-phần-cứng-hardware-acceptance-filter)
  - [3.7. Hệ thống API CAN của Zephyr RTOS](#37-hệ-thống-api-can-của-zephyr-rtos)
  - [3.8. Đối chiếu cú pháp: Zephyr RTOS vs STM32 HAL (STM32CubeIDE)](#38-đối-chiếu-cú-pháp-zephyr-rtos-vs-stm32-hal-stm32cubeide)
  - [3.9. Ba điều Zephyr làm khác mà lập trình viên cần nhớ](#39-ba-điều-zephyr-làm-khác-mà-lập-trình-viên-cần-nhớ)
- [4. Cấu hình Project CAN trong Zephyr (Devicetree & Kconfig)](#4-cấu-hình-project-can-trong-zephyr-devicetree--kconfig)
- [5. Cấu trúc thư mục dự án](#5-cấu-trúc-thư-mục-dự-án)
- [6. Danh mục 3 bài Demo nền tảng (demos/)](#6-danh-mục-3-bài-demo-nền-tảng-demos)
  - [Demo 1: lab3_demo1_loopback - Gửi nhận CAN chế độ Loopback nội bộ](#demo-1-lab3_demo1_loopback---gửi-nhận-can-chế-độ-loopback-nội-bộ)
  - [Demo 2: lab3_demo2_filters - Kiểm nghiệm bộ lọc phần cứng ID/Mask và RTR](#demo-2-lab3_demo2_filters---kiểm-nghiệm-bộ-lọc-phần-cứng-idmask-và-rtr)
  - [Demo 3: lab3_demo3_two_nodes - Giao tiếp giữa 2 Node trên Bus thật & Giám sát lỗi](#demo-3-lab3_demo3_two_nodes---giao-tiếp-giữa-2-node-trên-bus-thật--giám-sát-lỗi)
- [7. Danh mục 5 bài tập thực hành ứng dụng (lab3_ex*)](#7-danh-mục-5-bài-tập-thực-hành-ứng-dụng-lab3_ex)
  - [Bài 1: lab3_ex1_can_sensor_loopback - Đóng gói dữ liệu cảm biến đa byte Little-Endian](#bài-1-lab3_ex1_can_sensor_loopback---đóng-gói-dữ-liệu-cảm-biến-đa-byte-little-endian)
  - [Bài 2: lab3_ex2_can_command_rtr - Khung yêu cầu từ xa RTR và Điều khiển thiết bị](#bài-2-lab3_ex2_can_command_rtr---khung-yêu-cầu-từ-xa-rtr-và-điều-khiển-thiết-bị)
  - [Bài 3: lab3_ex3_can_filter_priority - Phân xử ưu tiên bus giữa Khẩn cấp và Định kỳ](#bài-3-lab3_ex3_can_filter_priority---phân-xử-ưu-tiên-bus-giữa-khẩn-cấp-và-định-kỳ)
  - [Bài 4: lab3_ex4_can_heartbeat_ping - Giám sát sống còn Node và Giao thức Ping-Pong](#bài-4-lab3_ex4_can_heartbeat_ping---giám-sát-sống-còn-node-và-giao-thức-ping-pong)
  - [Bài 5: lab3_ex5_distributed_traffic_light - Đèn giao thông phân tán Master - Slave](#bài-5-lab3_ex5_distributed_traffic_light---đèn-giao-thông-phân-tán-master---slave)
- [8. Hướng dẫn biên dịch, Nạp chương trình & Đấu dây thực nghiệm](#8-hướng-dẫn-biên-dịch-nạp-chương-trình--đấu-dây-thực-nghiệm)
- [9. Tổng kết các lỗi kỹ thuật thường gặp & Bảng Cheat-sheet](#9-tổng-kết-các-lỗi-kỹ-thuật-thường-gặp--bảng-cheat-sheet)

---

## 🎯 1. Mục tiêu thí nghiệm

Sau bài thực hành này, người học có thể:
1. **Làm chủ tầng vật lý và giao thức CAN:** Giải thích nguyên lý tín hiệu vi sai, tính chất dominant/recessive, vai trò triệt tiêu sóng phản xạ của điện trở kết cuối $120\,\Omega$ và cấu trúc chi tiết một khung tin CAN.
2. **Cấu hình ngoại vi bxCAN:** Tích hợp bộ điều khiển `can1` của Kit BKIT ARM4 thông qua Devicetree (`app.overlay`) và Kconfig (`prj.conf`) của Zephyr RTOS.
3. **Thành thạo API CAN của Zephyr:**
   - Gửi và nhận frame bằng API CAN ở chế độ Loopback và chế độ bus bình thường (Normal).
   - Thiết lập bộ lọc phần cứng (Hardware Acceptance Filter) theo cơ chế ID/Mask để phân luồng dữ liệu vào Message Queue mà không ngốn tài nguyên CPU.
4. **Đối chiếu đa nền tảng:** Chuyển đổi thành thạo giữa API Zephyr và thư viện STM32 HAL (`HAL_CAN_...`) mà STM32CubeIDE sinh ra.
5. **Xây dựng mạng phân tán 2 Kit:** Kết nối 2 kit vi điều khiển vật lý qua mạng bus, xử lý lỗi bus-off và xây dựng hệ thống đèn giao thông phân tán có cơ chế bảo an Fail-Safe.

---

## 🔌 2. Chuẩn bị phần cứng CAN trên Kit BKIT ARM4

- **Phần cứng tích hợp:** Kit BKIT ARM4 tích hợp sẵn bộ điều khiển **bxCAN1** bên trong vi điều khiển STM32F407ZGT6 và chip chuyển đổi tầng vật lý (Transceiver) **TJA1050**:
  - `PB8` nối với chân `CAN_RX` của TJA1050.
  - `PB9` nối với chân `CAN_TX` của TJA1050.
  - Ngõ ra của TJA1050 đưa ra cầu đấu vít: **CAN_H** và **CAN_L**.
- **Công tắc điện trở kết cuối SW13:** Trên kit có sẵn điện trở kết cuối $120\,\Omega$ nối tiếp với công tắc gạt **SW13**. Khi gạt sang **ON**, điện trở $120\,\Omega$ được nối song song giữa CANH và CANL.
- **Yêu cầu thí nghiệm:**
  - Demo 1–2 và Bài 1–3: Chỉ cần **1 kit** (chạy ở chế độ `CAN_MODE_LOOPBACK`).
  - Demo 3 và Bài 4–5: Làm theo cặp **2 kit** (chạy ở chế độ `CAN_MODE_NORMAL` trên bus thật).

---

## 📚 3. Kiến thức nền tảng về CAN (Toàn diện theo Tài liệu hướng dẫn)

### 3.1. Tầng vật lý: Tín hiệu vi sai
Bus CAN gồm hai dây dẫn: **CANH** và **CANL**. Thông tin truyền trên bus được xác định bằng hiệu điện áp:
$$V_{diff} = V_{CANH} - V_{CANL}$$

```
    Điện áp (V)
       3.5 V ──┐                           ┌──  CANH (3.5 V)
               │                           │
       2.5 V ──┼───────────────────────────┼──  Trạng thái Recessive (V_diff ≈ 0 V)
               │                           │
       1.5 V ──┘                           └──  CANL (1.5 V)
               ◄── Bit Dominant (0) ──────► ◄── Bit Recessive (1) ──►
               (V_diff = 3.5 - 1.5 = 2.0 V)   (V_diff = 2.5 - 2.5 = 0 V)
```

1. **Bit Dominant (Mức logic `0`):** Transceiver tích cực kéo CANH lên $\approx 3.5\text{V}$ và kéo CANL xuống $\approx 1.5\text{V} \implies V_{diff} \approx 2.0\text{V}$.
2. **Bit Recessive (Mức logic `1`):** Transceiver thả nổi (High-Z), điện trở nội kéo cả hai dây về mức điện áp chung $\approx 2.5\text{V} \implies V_{diff} \approx 0\text{V}$.
* **Khả năng chống nhiễu vượt trội:** Do thông tin nằm ở hiệu điện áp $V_{diff}$, bất kỳ xung nhiễu điện từ trường nào từ môi trường công nghiệp tác động đồng thời lên cả hai dây dẫn sẽ bị triệt tiêu hoàn toàn khi đi vào bộ so sánh vi sai của Transceiver.
* **Nguyên lý "Wired-AND":** Khi nhiều node cùng truyền, **chỉ cần một node phát bit `0` (Dominant)** thì toàn bộ bus sẽ mang giá trị `0`. Đặc tính này là nền tảng cốt lõi của cơ chế phân xử bus và phát bit xác nhận ACK.
* **Điện trở kết cuối ($120\,\Omega$):** Mạng bus CAN bắt buộc phải có đúng hai điện trở $120\,\Omega$ tại hai đầu mút xa nhất của bus để hấp thụ sóng phản xạ tín hiệu điện:
  $$R_{bus} = 120\,\Omega \parallel 120\,\Omega = 60\,\Omega$$
  Nếu thiếu kết cuối, tín hiệu phản xạ gây biến dạng xung và phát sinh lỗi ở tốc độ cao; nếu thừa kết cuối (quá nhiều điện trở), tải tăng cao làm giảm biên độ điện áp vi sai.

---

### 3.2. Cấu trúc Frame CAN

Một khung dữ liệu CAN chuẩn (Standard Data Frame) gồm các trường:
```
┌─────┬────────────────┬───────┬────────────┬─────────────┬───────────┬─────────┬─────┐
│ SOF │ Identifier 11b │ RTR 1b│ Control 6b │ Data 0..8 B │  CRC 16b  │ ACK 2b  │ EOF │
└─────┴────────────────┴───────┴────────────┴─────────────┴───────────┴─────────┴─────┘
```
* **Không có địa chỉ đích:** CAN không định danh theo địa chỉ của node nhận hay node gửi. Mỗi frame mang một **Identifier (ID)** mô tả nội dung của frame (ví dụ "Nhiệt độ động cơ", "Lệnh phanh khẩn cấp"). Mọi node trên bus đều nhận được và tự quyết định xem có dùng hay không thông qua bộ lọc phần cứng.
* **Độ dài Identifier:**
  * **Standard ID (CAN 2.0A):** 11 bits (giá trị từ `0x000` đến `0x7FF`).
  * **Extended ID (CAN 2.0B):** 29 bits (gồm 11 bit Base ID + 18 bit Extended ID, cờ `CAN_FRAME_IDE`).
* **Frame yêu cầu từ xa (RTR Frame):** Cờ `CAN_FRAME_RTR` tích cực, không mang trường dữ liệu (Data Field = 0 bytes), dùng để hỏi từ xa: *"Ai có dữ liệu mang ID này thì hãy gửi lên bus"*.

---

### 3.3. Phân xử bus theo ID
Khi nhiều node bắt đầu gửi đồng thời tại cùng một thời điểm:
* Mỗi node vừa gửi từng bit của ID, vừa đọc ngược lại mức logic thực tế trên đường truyền bus.
* Nhờ đặc tính bit `0` (Dominant) đè bẹp bit `1` (Recessive): Node nào phát bit `1` nhưng đọc thấy mức `0` sẽ biết mình đã thua cuộc phân xử. Node đó **lập tức ngừng phát** và trở thành node nhận.
* Node mang **ID có giá trị số nhỏ nhất** sẽ phát ra các bit `0` sớm hơn $\implies$ **Luôn giành chiến thắng phân xử** và frame của nó tiếp tục truyền đi nguyên vẹn mà không xảy ra va chạm hay mất dữ liệu.
* *Nguyên tắc thiết kế hệ thống:* Các thông điệp báo động khẩn cấp, bảo vệ an toàn luôn được gán ID nhỏ nhất.

---

### 3.4. Định thời bit (Bit Timing)
Mỗi bit được chia thành các khe thời gian nhỏ gọi là **Time Quantum ($t_q$)**:
```
┌──────────┬──────────────┬──────────────┬──────────────┐
│ SYNC_SEG │   PROP_SEG   │  PHASE_SEG1  │  PHASE_SEG2  │
└──────────┴──────────────┴──────────────┴──────────────┘
           ◄──────── Time Segment 1 ─────►◄── Seg 2 ────►
                                         ▲
                                    Sample Point (Lấy mẫu tại 75% - 87.5%)
```
* Bộ điều khiển lấy mẫu giá trị bit tại **Điểm lấy mẫu (Sample Point)**, thường nằm ở khoảng $75\% - 87.5\%$ độ dài của bit.
* Mọi node tham gia mạng bus **bắt buộc phải cấu hình cùng một bitrate** (trong bài này là $500\text{ kbit/s}$).
* Với STM32CubeIDE bạn phải tự tính toán prescaler và số $t_q$ của từng đoạn; với **Zephyr RTOS**, bạn chỉ cần khai báo `bus-speed = <500000>;` trong Devicetree, driver sẽ tự động tính toán chính xác các phân đoạn bit.

---

### 3.5. ACK và Xử lý lỗi Bus
* Mỗi node nhận đúng một frame (kiểm tra CRC trùng khớp) sẽ ghi một bit Dominant (`0`) vào **khe ACK (ACK Slot)**.
* Nếu node phát không thấy bit ACK (khe ACK vẫn ở mức Recessive `1`), frame được coi là truyền lỗi và bộ điều khiển sẽ tự động phát lại.
* **Hệ quả quan trọng:** Một node đứng một mình trên bus sẽ **không bao giờ gửi thành công**, vì không có bất kỳ node nào khác trên bus kéo bit ACK!
* **Bộ đếm lỗi và chuyển trạng thái:** Mỗi node duy trì bộ đếm lỗi phát **TEC (Transmit Error Counter)** và lỗi nhận **REC (Receive Error Counter)**:
  - $TEC, REC < 128$: Trạng thái **`ERROR-ACTIVE`** (Bình thường).
  - $TEC$ hoặc $REC \ge 128$: Trạng thái **`ERROR-PASSIVE`** (Cảnh báo lỗi, chỉ phát Passive Error Flag).
  - $TEC > 255$: Trạng thái **`BUS-OFF`** (Ngắt kết nối hoàn toàn, bị cách ly khỏi bus để tránh làm nghẽn mạng).

---

### 3.6. Bộ lọc nhận phần cứng (Hardware Acceptance Filter)
Bộ điều khiển `bxCAN` trên STM32F4 tích hợp các bộ lọc phần cứng độc lập:
* Frame nhận về không khớp với bất kỳ bộ lọc nào sẽ bị **loại bỏ ngay trong phần cứng**, CPU hoàn toàn không tốn công xử lý hay bị gián đoạn.
* Mỗi bộ lọc gồm một cặp giá trị: **ID** và **Mask**:
  - Bit tại vị trí nào trong `Mask` bằng **`1`**: Bit ID tương ứng nhận về bắt buộc phải khớp chính xác với `Filter_ID`.
  - Bit tại vị trí nào trong `Mask` bằng **`0`**: Bỏ qua (**Don't care** - có thể là 0 hoặc 1 đều được nhận).
* *Ví dụ:*
  - `id = 0x123`, `mask = 0x7FF` (`CAN_STD_ID_MASK`): Chỉ nhận duy nhất 1 ID `0x123`.
  - `id = 0x100`, `mask = 0x7F0`: Nhận toàn bộ dải gồm 16 ID từ `0x100` đến `0x10F`.

---

### 3.7. Hệ thống API CAN của Zephyr RTOS

#### Bảng tra cứu các API CAN quan trọng:
| Hàm API / Cấu trúc | Ý nghĩa chức năng |
| :--- | :--- |
| `DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))` | Lấy con trỏ thiết bị CAN được chọn trong Devicetree (`can1`) |
| `can_set_mode(dev, mode)` | Cấu hình chế độ: `CAN_MODE_NORMAL`, `CAN_MODE_LOOPBACK`, `CAN_MODE_LISTENONLY` (chỉ gọi khi controller đang dừng) |
| `can_start(dev)` / `can_stop(dev)` | Bắt đầu / dừng tham gia mạng bus (`can_start` cũng đồng thời reset bộ đếm lỗi TEC/REC) |
| `struct can_frame` | Chứa thông tin khung tin: `id`, `dlc`, `flags` (`CAN_FRAME_IDE`, `CAN_FRAME_RTR`), `data[8]` |
| `can_send(dev, &frame, timeout, cb, user_data)` | Gửi frame. `cb = NULL`: chặn tới khi gửi xong; `cb` khác NULL: trả về ngay, `cb` được gọi trong ISR khi hoàn tất |
| `struct can_filter` | Cấu hình bộ lọc: `id`, `mask`, `flags` (`CAN_FILTER_IDE` cho ID 29-bit) |
| `CAN_MSGQ_DEFINE(q, n)` + `can_add_rx_filter_msgq()` | Frame khớp bộ lọc được tự động đẩy vào Message Queue để thread xử lý |
| `can_add_rx_filter(dev, cb, user_data, &filter)` | Frame khớp bộ lọc sẽ trực tiếp gọi callback trong ngữ cảnh ngắt (ISR) |
| `can_get_state(dev, &state, &err_cnt)` | Đọc trạng thái bus và giá trị bộ đếm lỗi TEC/REC |
| `can_set_state_change_callback(dev, cb, data)` | Đăng ký callback (ISR) thông báo mỗi khi trạng thái bus thay đổi |

#### Bảng phân biệt 3 chế độ hoạt động của CAN Controller:
| Chế độ (`can_mode`) | Hành vi hoạt động | Trường hợp ứng dụng |
| :--- | :--- | :--- |
| **`CAN_MODE_NORMAL`** | Gửi/nhận trên bus thật, bắt buộc cần node khác kéo bit ACK. Không nhận lại frame của chính mình | Demo 3, Bài tập 4 và Bài tập 5 (Nối 2 Kit) |
| **`CAN_MODE_LOOPBACK`**| Frame gửi đi được nhận lại ngay lập tức trong bộ điều khiển, không cần node khác ACK | Demo 1, Demo 2, Bài tập 1, 2, 3 (Chỉ cần 1 Kit) |
| **`CAN_MODE_LISTENONLY`**| Chỉ lắng nghe, hoàn toàn không ghi lên bus (kể cả bit ACK) | Thiết bị phân tích mạng, bộ bắt gói tin ("Sniffer") |

---

### 3.8. Đối chiếu cú pháp: Zephyr RTOS vs STM32 HAL (STM32CubeIDE)

| Thao tác kỹ thuật | Zephyr RTOS v4.4.0 | STM32 HAL (`HAL_CAN_...`) trong STM32CubeIDE |
| :--- | :--- | :--- |
| **Cấu hình Bitrate** | `bus-speed = <500000>;` trong file `app.overlay` | `hcan1.Init.Prescaler = 6;`<br>`hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;`<br>`hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;`<br>`HAL_CAN_Init(&hcan1);` |
| **Chế độ Loopback** | `can_set_mode(dev, CAN_MODE_LOOPBACK);` | `hcan1.Init.Mode = CAN_MODE_LOOPBACK;` |
| **Cấu hình Bộ lọc** | `struct can_filter f = {.id = 0x100, .mask = 0x7F0};`<br>`can_add_rx_filter_msgq(dev, &q, &f);` | `CAN_FilterTypeDef f;`<br>`f.FilterMode = CAN_FILTERMODE_IDMASK;`<br>`f.FilterScale = CAN_FILTERSCALE_32BIT;`<br>`f.FilterIdHigh = 0x100 << 5;`<br>`f.FilterMaskIdHigh = 0x7F0 << 5;`<br>`HAL_CAN_ConfigFilter(&hcan1, &f);` |
| **Khởi động Controller**| `can_start(dev);` | `HAL_CAN_Start(&hcan1);` |
| **Gửi Frame** | `can_send(dev, &frame, K_MSEC(100), NULL, NULL);` | `CAN_TxHeaderTypeDef h = {.StdId = 0x123, ...};`<br>`HAL_CAN_AddTxMessage(&hcan1, &h, data, &mailbox);` |
| **Nhận bằng Ngắt** | `can_add_rx_filter(dev, rx_cb, NULL, &f);` | `HAL_CAN_ActivateNotification(&hcan1, ...);`<br>`HAL_CAN_RxFifo0MsgPendingCallback();` |
| **Giám sát Trạng thái / Lỗi**| `can_get_state();`<br>`can_set_state_change_callback();` | `HAL_CAN_GetState();`<br>`HAL_CAN_GetError();`<br>`HAL_CAN_ErrorCallback(&hcan1);` |

---

### 3.9. Ba điều Zephyr làm khác mà lập trình viên cần nhớ

> [!IMPORTANT]
> 1. **Khung tin RTR bị loại bỏ mặc định:** Trong Zephyr RTOS, driver CAN mặc định bỏ qua các frame yêu cầu từ xa RTR. Muốn nhận được frame RTR, **bắt buộc phải bật cấu hình `CONFIG_CAN_ACCEPT_RTR=y` trong file `prj.conf`** (Demo 2 và Bài tập 2 thể hiện rõ điều này).
> 2. **`can_send()` với callback `NULL` sẽ bị CHẶN VÔ HẠN:** Khi gọi `can_send()` với callback bằng `NULL` kèm timeout `K_FOREVER`, hàm sẽ chặn chờ cho đến khi frame được truyền thành công (có node ACK). Trên mạng bus thật nếu không có node nào kéo ACK, lệnh này sẽ **không bao giờ trả về** và làm treo luồng. Do đó ở Demo 3 và các bài tập làm việc theo cặp kit, ta luôn sử dụng hàm gửi kèm callback non-blocking hoặc timeout xác định.
> 3. **Bus-off tự động phục hồi:** Driver CAN của STM32 trong Zephyr mặc định kích hoạt chế độ tự phục hồi (*Auto-recovery*) khi bị Bus-off, trừ khi người dùng chủ động cấu hình cờ `CAN_MODE_MANUAL_RECOVERY`.

---

## ⚙️ 4. Cấu hình Project CAN trong Zephyr (Devicetree & Kconfig)

### 4.1. File Devicetree overlay (`app.overlay`)
Kích hoạt bộ điều khiển `can1`, gán chân `PB8/PB9` và thiết lập bitrate $500\text{ kbit/s}$:
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
    bus-speed = <500000>;
};
```

### 4.2. File Kconfig (`prj.conf`)
Bật driver CAN trong nhân Kernel:
```ini
CONFIG_CAN=y
CONFIG_CAN_ACCEPT_RTR=y
CONFIG_CAN_MAX_FILTER=10
```

---

## 📁 5. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab3_canbus)
├── demos/                              # 3 Bài Demo khảo sát nguyên lý mạng CAN
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
└── README.md                           # Tài liệu kỹ thuật toàn diện Lab 3
```

---

## 🔬 6. Danh mục 3 bài Demo nền tảng (`demos/`)

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

## 🛠️ 7. Danh mục 5 bài tập thực hành ứng dụng (`lab3_ex*`)

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

## 💻 8. Hướng dẫn biên dịch, Nạp chương trình & Đấu dây thực nghiệm

### 8.1. Biên dịch và nạp firmware
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

### 8.2. Hướng dẫn đấu dây khi chạy Bus thật giữa 2 Kit:
Nối dây giữa cầu đấu vít CAN của Kit A và Kit B:
* **CAN_H** của Kit A nối với **CAN_H** của Kit B.
* **CAN_L** của Kit A nối với **CAN_L** của Kit B.
* **GND** của Kit A nối với **GND** của Kit B (Bắt buộc để ổn định điện thế chung).
* Gạt công tắc **SW13** trên cả 2 kit sang vị trí **ON** để kích hoạt trở kết cuối $120\,\Omega$.

---

## ⚠️ 9. Tổng kết các lỗi kỹ thuật thường gặp & Bảng Cheat-sheet

1. **Lỗi không gửi được frame (`can_send` trả về `-EAGAIN` hoặc `-ENETUNREACH`):**
   * Chưa gọi lệnh `can_start(can_dev)`.
   * Ở chế độ bus thật: Chưa có node thứ hai ACK gói tin, hoặc chưa gạt công tắc trở kết cuối SW13.
2. **Lỗi không nhận được frame:**
   * Bộ lọc cấu hình sai `id` hoặc `mask`.
   * Quên thêm cờ `CAN_FILTER_IDE` khi lọc Identifier 29-bit mở rộng.
3. **Lỗi tràn hàng đợi Message Queue:**
   * Hàng đợi khai báo trong `CAN_MSGQ_DEFINE` có kích thước quá nhỏ (ví dụ chỉ có 2-4 phần tử), trong khi bus nhận nhiều frame liên tục. Nên cấu hình tối thiểu 8 đến 16 phần tử.
