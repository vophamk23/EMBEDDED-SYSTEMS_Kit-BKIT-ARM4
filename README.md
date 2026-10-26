# LAB 3: CONTROLLER AREA NETWORK (CAN BUS) AND DISTRIBUTED SYSTEMS

Khoa Khoa hoc va Ky thuat May tinh - Truong Dai hoc Bach Khoa, DHQG-HCM  
Mon hoc: He thong nhung (CO3037 / CO3038)  
Nhanh Git: `lab3_canbus`  
Nen tang: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
He dieu hanh: Zephyr RTOS v4.4.0  

---

## 1. Muc tieu thi nghiem

1. Nam vung kien truc va nguyen ly truyen thong mang vi dieu khien cong nghiep Controller Area Network (CAN Bus 2.0A/B theo tieu chuan ISO 11898):
   - Co che phan xu duong truyen khong pha huy (Non-destructive Bitwise Arbitration) dua tren Identifier.
   - Cau truc cac loai khung CAN: Khung du lieu (Data Frame), Khung yeu cau tu xa (Remote Transmission Request - RTR Frame), Khung bao loi (Error Frame).
   - Bo dieu khien CAN STM32 bxCAN: FIFO RX, Mailbox TX, Bit Timing (Prescaler, Phase Segments, SJW) dat toc do chuan 250 kbps hoac 500 kbps.
2. Thiet lap bo loc phan cung (Hardware Acceptance Filters & Masks) bang API `can_add_rx_filter()` cua Zephyr de giam tai xu ly cho CPU.
3. Chuyen doi linh hoat giua che do kiem thu noi bo (Loopback Mode) va che do mang thuc te da node (Normal Bus Mode).
4. Thiet ke he thong phan tan: Giam sat song con (Heartbeat Monitoring) va he thong den giao thong dong bo giua 2 Kit Master - Slave.

---

## 2. Phan bo chan phan cung va So do dau noi (Hardware Setup)

### 2.1. Cau hinh chan CAN1 tren STM32F407
File `app.overlay` gan cac chan Alternate Function cho ngoai vi CAN1:

| Tin hieu | Chan GPIO STM32F407 | Chuc nang Alternate Function |
| :--- | :--- | :--- |
| CAN1_RX | `PB8` | AF9 (CAN1) |
| CAN1_TX | `PB9` | AF9 (CAN1) |
| LED3 / Output Y0 / Y1 | `PE4` / `PE5` / `PE6` | Ngo ra chi thi trang thai |
| Console UART | `PA9` / `PA10` | USART1 (115200 bps) |

### 2.2. So do dau noi giua 2 Kit BKIT ARM4 (Giao tiep Bus that)
Khi chay cac bai thuc hanh giua 2 Kit phan cung (`lab3_demo3`, `lab3_ex4`, `lab3_ex5`), phai dau noi qua bo thu phat CAN Transceiver (TJA1050 hoac SN65HVD230):

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

**Luu y ky thuat bat buoc:**
- Phai bat dien tro dau cuoi 120 Ohm (Terminating Resistor) o ca 2 dau duong truyen de tranh phan xa tin hieu tren duong day vi sai.
- Phai noi chung day Mass (GND) giua 2 board de thiet lap dien the tham chieu on dinh cho tin hieu CANH va CANL.

---

## 3. Danh muc cac bai thuc hanh va Mo ta chi tiet

### 3.1. Cac bai Demo nen tang (`demos/`)

- `lab3_demo1_loopback`:
  - Kich hoat che do `CAN_MODE_LOOPBACK`.
  - Node tu dong dong goi va gui khung CAN ID `0x123` gom 4 byte payload; sau do tu thu lai qua bo dem RX de kiem tra tinh toan ven cua stack truyen thong Zephyr.
- `lab3_demo2_filters`:
  - Cung cap vi du ve cau hinh bo loc phan cung:
    - Filter 1: Bat dac biet ID `0x100` voi Mask tuyet doi `0x7FF`.
    - Filter 2: Bat nhom ID tu `0x200` den `0x20F` voi Mask dai `0x7F0`.
  - Chung minh cac khung co ID khong khop voi bo loc se bi phan cung huy bo ngay lap tuc ma khong kich hoat ngat CPU.
- `lab3_demo3_two_nodes`:
  - Giao tiep giua 2 kit tren bus that (`CAN_MODE_NORMAL`).
  - Kit A mang `NODE_ID = 1`, Kit B mang `NODE_ID = 2`.
  - Moi giay moi node phat Heartbeat ID `0x700 + NODE_ID` va nhan Heartbeat cua doi phuong; dong thoi theo doi trang thai loi bus qua `can_get_state()`.

---

### 3.2. Cac bai tap ung dung (`lab3_ex*`)

#### Bai 1: `lab3_ex1_can_sensor_loopback` - Dong goi va Giai ma du lieu cam bien
- **Yeu cau:** Mo phong thu thap du lieu da cam bien va truyen qua mang CAN Loopback.
- **Dinh dang khung du lieu (Payload 8 bytes, ID: 0x200):**
  - Byte 0-1: Gia tri Nhiet do (`int16_t`, don vi: 0.1°C).
  - Byte 2: Do am tuong doi (`uint8_t`, %).
  - Byte 3-4: Ap suat khi quyen (`uint16_t`, hPa).
  - Byte 5: Ma trang thai he thong (`uint8_t`).
  - Byte 6-7: Checksum / Counter tang dan.
- **Thuc thi:** Dong goi dung quy chuan Byte-order (Little Endian), gui qua CAN, giai ma o ham callback nhan va in thong so len UART console.

#### Bai 2: `lab3_ex2_can_command_rtr` - Dieu khien lenh va Khung yeu cau tu xa (RTR)
- **Yeu cau:** Thuc hien co che hoi/dap trang thai khong can gui du lieu yeu cau.
- **Thiet ke:**
  - Node Master gui mot khung RTR Frame (`flags = CAN_FRAME_RTR`, `dlc = 0`, ID: `0x301`).
  - Node Slave cau hinh bo loc bat RTR Frame tren ID `0x301`. Khi nhan duoc yeu cau, Slave tu dong phan hoi lai mot Data Frame (chua trang thai bat/tat cua Role va cac ngo ra Y0/Y1).
  - Dong thoi Master co the gui Data Frame co ID `0x300` chua lenh bat/tat den truc tiep.

#### Bai 3: `lab3_ex3_can_filter_priority` - Bo loc phan cung va Phan xu uu tien ID
- **Yeu cau:** Thuc nghiem nguyen ly phan xu bitwise arbitration.
- **Thiet ke:**
  - Hai luong truyen dong thoi phat 2 khung CAN vao cung mot thoi diem:
    - Goi tin Khan cap (Alarm/Emergency): ID `0x080` (Uu tien rat cao).
    - Goi tin Dinh ky (Telemetry): ID `0x400` (Uu tien thap).
  - Ket qua thu nghiem: Khung ID `0x080` luon luon chanh quyen va duoc truyen xong truoc tren bus CAN ma khong bi mat mat du lieu.
  - Cung voi do, bo loc Mask duoc cau hinh de chi chap nhan cac ban tin hop le.

#### Bai 4: `lab3_ex4_can_heartbeat_ping` - Giam sat song con Ping-Pong 2 Node
- **Yeu cau:** Kiem soat trang thai ket noi vat ly tren bus CAN giua 2 kit doc lap.
- **Thiet ke:**
  - Node 1 (Kit A) dinh ky 1000 ms phat ban tin Heartbeat ID `0x701`.
  - Node 2 (Kit B) khoi tao timer giam sat 2500 ms:
    - Moi khi nhan duoc Heartbeat tu Kit A, timer duoc reset.
    - Neu Kit A bi rut nguon hoac dut day CAN, timer het han, Kit B lap tuc kich hoat den LED canh bao "NODE 1 TIMEOUT / OFFLINE".
  - Chuc nang mo rong: Giao thuc Ping-Pong do thoi gian tre khu hoi (Round-Trip Time - RTT) qua CAN bus.

#### Bai 5: `lab3_ex5_distributed_traffic_light` - He thong den giao thong phan tan qua CAN
- **Yeu cau:** Xay dung mo hinh he thong giao thong thong minh phan tan tren 2 Kit:
  - **Kit A (Controller Node - Master):**
    - Chay FSM giao thong (Do 7s, Xanh 5s, Vang 2s).
    - Tinh toan thoi gian dem nguoc thuc te moi giay.
    - Dong goi trang thai: `{ uint8_t phase_id; uint8_t remaining_sec; }` va broadcast len bus CAN voi ID `0x250`.
  - **Kit B (Display Node - Slave):**
    - Dat tai hien truong, nhan khung CAN ID `0x250`.
    - Dong bo hoan toan ngo ra den LED3, Y0, Y1 va in thong tin dem nguoc ra cong UART.
    - Co che an toan (Fail-safe): Neu mat ket noi CAN voi Master qua 3 giay, Slave tu dong chuyen sang che do den Vang chop tat canh bao giao thong nguy hiem.

---

## 4. Huong dan Bien dich va Nap chuong trinh

### 4.1. Chay che do Loopback tren 1 Kit
```bash
west build -p always -b stm32f4_disco lab3_ex1_can_sensor_loopback
west flash
```

### 4.2. Chay che do 2 Kit tren Bus that
1. **Nap cho Kit A (Node 1 / Master):**
   - Mo file `main.c`, dam bao dinh nghia `#define NODE_ID 1` (hoac macro vai tro Master).
   - Cam ST-Link vao Kit A va chay:
     ```bash
     west build -p always -b stm32f4_disco lab3_ex5_distributed_traffic_light
     west flash
     ```

2. **Nap cho Kit B (Node 2 / Slave):**
   - Mo file `main.c`, sua dinh nghia `#define NODE_ID 2` (hoac macro vai tro Slave).
   - Cam ST-Link sang Kit B va chay:
     ```bash
     west build -p always -b stm32f4_disco lab3_ex5_distributed_traffic_light
     west flash
     ```

3. **Kiem tra:** Mo 2 cua so Serial Monitor (UART) cho ca 2 kit o toc do 115200 de giam sat dong bo.

---

## 5. Quy tac ky thuat va Luu y ve CAN Bus

1. **Cau hinh Kconfig bat buoc:** Trong `prj.conf`, luon can co `CONFIG_CAN=y` va `CONFIG_CAN_MAX_FILTER=5` de kich hoat driver CAN tren STM32.
2. **Che do Bus Mode:** Khong the truyen nhan giua 2 kit neu mot ben dang bat `CAN_MODE_LOOPBACK`. Phai chuyen sang `CAN_MODE_NORMAL` (hoac dat flags = 0).
3. **Loi Bus-Off:** Neu day truyen tin bi hoac khong co tro 120 Ohm, bo dieu khien CAN se tich luy loi (Error Passive) va chuyen sang trang thai `CAN_STATE_BUS_OFF`. Can kiem tra ky phan cung truoc khi chay thuc nghiem.
