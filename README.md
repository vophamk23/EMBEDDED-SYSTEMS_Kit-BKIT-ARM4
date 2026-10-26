# LAB 1: MULTITHREADING AND REAL-TIME SCHEDULING IN ZEPHYR RTOS

Khoa Khoa hoc va Ky thuat May tinh - Truong Dai hoc Bach Khoa, DHQG-HCM  
Mon hoc: He thong nhung (CO3037 / CO3038)  
Nhanh Git: `lab1_multithreading`  
Nen tang: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
He dieu hanh: Zephyr RTOS v4.4.0  

---

## 1. Muc tieu thi nghiem

1. Nam vung kien truc da luong (Multithreading) va bo lap lich thoi gian thuc cua Zephyr RTOS.
2. Phan biet co che khai bao luong tinh (`K_THREAD_DEFINE`) va khai bao luong dong (`k_thread_create`).
3. Hieu ro phan cap do uu tien:
   - Cooperative threads: Do uu tien am (tu -15 den -1), khong the bi chiem quyen (non-preemptible) tru khi chu dong goi `k_yield()` hoac sleep.
   - Preemptive threads: Do uu tien khong am (tu 0 den 14), co the bi luong co do uu tien cao hon ngat va chiem CPU bat ky luc nao.
4. Thiet lap bo nho ngan xep (Stack allocation) va bao ve chong tran stack (`CONFIG_STACK_SENTINEL` / `CONFIG_HW_STACK_PROTECTION`).
5. Ap dung ky thuat truyen tham so cau hinh cho luong (`struct blink_cfg`) va dieu khien may trang thai huu han (FSM) ket hop da luong doc lap.

---

## 2. Phan bo chan phan cung (Pinout Configuration)

He thong su dung file Devicetree overlay (`app.overlay`) de dinh nghia lai cac chan ngoai vi tren kit BKIT ARM4:

| Ngoai vi / Tin hieu | Chan GPIO STM32F407 | Alias Devicetree | Che do hoat dong |
| :--- | :--- | :--- | :--- |
| LED3 (DEBUG LED) | `PE4` | `led0` | GPIO Output, Active-High |
| Output Y0 | `PE5` | `led1` | GPIO Output, Active-High |
| Output Y1 | `PE6` | `led2` | GPIO Output, Active-High |
| Console Debug | `PA9` / `PA10` | `usart1` | Baudrate 115200 bps, 8-N-1 |

---

## 3. Danh muc cac bai thuc hanh va Mo ta chi tiet

### 3.1. Cac bai Demo nen tang (`demos/`)

- `lab1_demo1_single_thread`:
  - Kiem tra luong mac dinh `main`.
  - Su dung `k_msleep()` de dinh thoi va in thong diep debug ra cong UART.
- `lab1_demo2_multi_thread`:
  - Khoi tao 2 luong cung chay song song voi cung mot muc do uu tien.
  - Quan sat hanh vi chia se thoi gian CPU thong qua Time-slicing va goi chu dong `k_yield()`.
- `lab1_demo3_priority`:
  - Khoi tao cac luong voi cac muc do uu tien khac nhau.
  - Chung minh nguyen tac: Luong co gia tri priority nho hon (uu tien cao hon) se luon chiem CPU truoc.
- `lab1_demo4_preemption`:
  - Kiem chung co che chiem quyen (Preemptive Scheduling).
  - Khi luong uu tien cao tinh giac (wake up), bo lap lich lap tuc tam ngung luong uu tien thap de phuc vu luong uu tien cao.

---

### 3.2. Cac bai tap ung dung (`lab1_ex*`)

#### Bai 0: `bkit_blinky` - Kiem tra phan cung kit
- **Yeu cau:** Kiem tra ket noi GPIO port E va LED3 tren board.
- **Thuc thi:** Nhay LED3 chu ky doi xung: Sang 1000 ms, Tat 1000 ms.
- **Kiem chung:** LED3 tren kit nhap nhay deu dan voi chu ky 2 giay.

#### Bai 1: `lab1_ex1_asym_blinky` - Nhay LED bat doi xung
- **Yeu cau:** Tao mot luong doc lap `thread_led_blinky` bang macro `K_THREAD_DEFINE`.
- **Thong so:**
  - Stack size: 1024 bytes.
  - Priority: 7 (Preemptive).
  - Chu ky: Sang 1000 ms (1 giay), Tat 2000 ms (2 giay). Tong chu ky 3 giay.
- **Muc dich:** Lam quen voi viec tach logic dieu khien khoi ham `main()` vao mot luong chuyen biet.

#### Bai 2: `lab1_ex2_sync_led_y0` - Dong bo qua bien toan cuc
- **Yeu cau:** Phoi hop hoat dong giua 2 luong thong qua bien toan cuc `volatile bool g_led_state`.
- **Thiet ke:**
  - Luong Master (`thread_led_blinky`): Dieu khien LED3 nhay chu ky Sang 2000 ms / Tat 2000 ms; dong thoi cap nhat trang thai logic vao `g_led_state`.
  - Luong Slave (`thread_y0_sync`): Chu ky polling 10 ms doc gia tri `g_led_state` va xuat logic tuong ung ra Output Y0 (`PE5`).
- **Luu y ky thuat:** Tu khoa `volatile` bat buoc phai co de trinh bien dich khong toi uu hoa bien ra thanh ghi, dam bao luong Slave luon doc truc tiep tu bo nho RAM.

#### Bai 3: `lab1_ex3_independent_patterns` - Mau nhay doc lap da luong
- **Yeu cau:** Xay dung 2 luong chay hoan toan doc lap voi chu ky va pha khong dong bo:
  - Luong 1 (`thread_led_blinky`): LED3 nhay Sang 1500 ms, Tat 500 ms (chu ky 2000 ms).
  - Luong 2 (`thread_y0_y1_pattern`): Dieu khien 2 ngo ra Y0 va Y1 lap tuan tu theo chu ky 5000 ms qua 3 pha:
    - Pha 1 (2000 ms): Y0 = 1, Y1 = 0.
    - Pha 2 (1500 ms): Y0 = 0, Y1 = 1.
    - Pha 3 (1500 ms): Y0 = 1, Y1 = 1.
- **Kiem chung:** Quan sat tren Logic Analyzer hoac he thong den LED ngoai de thay su doc lap tuyet doi giua 2 bo dinh thoi.

#### Bai 4: `lab1_ex4_multi_led_param` - Tai su dung ham Thread bang tham so
- **Yeu cau:** Khong viet lap lai ma nguon cho tung luong; su dung mot ham duy nhat `thread_blink_entry` de dieu khien dong thoi 3 den thong qua con tro cau truc:
  ```c
  struct blink_cfg {
      struct gpio_dt_spec led;
      int on_ms;
      int off_ms;
      const char *name;
  };
  ```
- **Thong so cau hinh:**
  - LED3 (`PE4`): Sang 500 ms, Tat 500 ms.
  - Output Y0 (`PE5`): Sang 1000 ms, Tat 1000 ms.
  - Output Y1 (`PE6`): Sang 1500 ms, Tat 1500 ms.
- **Ky thuat:** Khoi tao luong bang `K_THREAD_DEFINE` va truyen dia chi con tro `&cfg_led3`, `&cfg_y0`, `&cfg_y1` qua tham so `p1`.

#### Bai 5: `lab1_ex5_traffic_light` - He thong den giao thong da luong
- **Yeu cau:** Mo phong he thong den giao thong ket hop he thong giam sat song con (Heartbeat):
  - Luong 1 (Heartbeat): Dieu khien LED3 nhay deu dan 1000 ms ON / 1000 ms OFF nham bao hieu MCU van dang hoat dong binh thuong.
  - Luong 2 (May trang thai FSM giao thong):
    - Trang thai RED (Do - Den Y0): Sang 7000 ms.
    - Trang thai GREEN (Xanh - Den Y1): Sang 5000 ms.
    - Trang thai YELLOW (Vang - Bat ca Y0 va Y1): Sang 2000 ms.
- **Kiem chung:** FSM hoat dong chinh xac ve thoi gian ma khong lam anh huong den nhip dap cua Heartbeat.

---

## 4. Huong dan Bien dich va Nap chuong trinh

Tu thu muc goc cua nhanh `lab1_multithreading`, thuc hien cac lenh sau:

```bash
# Vi du bien dich va nap Bai 5: He thong den giao thong
west build -p always -b stm32f4_disco lab1_ex5_traffic_light

# Nap firmware qua ST-Link
west flash

# Mo cong UART de giam sat log (tren Linux)
picocom -b 115200 /dev/ttyUSB0
```

---

## 5. Quy tac ky thuat va Luu y

1. **Stack Size:** Do ham `printk()` va driver GPIO yeu cau ngan xep tuong doi, moi luong can duoc cap phat toi thieu 1024 bytes (`1024`) de tranh loi HardFault / Stack Overflow.
2. **Priority Range:** Cac luong thong thuong trong bai thuc hanh duoc dat trong khoang tu 5 den 8 (Preemptive). Luong giam sat hoac yeu cau dap ung nhanh dat priority thap hon ve mat gia tri (uu tien cao hon).
3. **Cooperative vs Preemptive:** Neu dat do uu tien am (vi du: `-1`), luong se chay lien tuc va bo lap lich khong the ngat neu luong khong chu dong nhuong quyen (`k_msleep` hoac `k_yield`).
