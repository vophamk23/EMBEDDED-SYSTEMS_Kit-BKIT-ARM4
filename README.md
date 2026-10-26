# LAB 2: SYNCHRONIZATION AND INTER-PROCESS COMMUNICATION (IPC) IN ZEPHYR RTOS

Khoa Khoa hoc va Ky thuat May tinh - Truong Dai hoc Bach Khoa, DHQG-HCM  
Mon hoc: He thong nhung (CO3037 / CO3038)  
Nhanh Git: `lab2_synchronization`  
Nen tang: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
He dieu hanh: Zephyr RTOS v4.4.0  

---

## 1. Muc tieu thi nghiem

1. Nam vung cac co che dong bo va truyen thong lien tien trinh (IPC) trong he dieu hanh thoi gian thuc Zephyr RTOS:
   - **Binary Semaphore (`k_sem`):** Dong bo 1-1 giua ngat phan cung (ISR) va luong thuc thi (Deferred Interrupt Processing).
   - **Counting Semaphore:** Quan ly ho chua tai nguyen huu han (Resource Pool) va dem su kien don dap.
   - **Mutex (`k_mutex`):** Loai tru tuong ho (Mutual Exclusion), ngan ngua xung dot du lieu (Race Condition) va co che ke thua do uu tien (Priority Inheritance).
   - **Message Queue (`k_msgq`):** Bo dem vong truyen nhan du lieu co cau truc kieu Producer - Consumer giua ISR va Thread hoac giua cac Thread.
2. Xu ly dung quy chuan ngat ngoai vi (GPIO ISR): Khong block trong ISR, su dung `K_NO_WAIT` va chuyen viec nang sang luong thuc thi (Deferred Work).
3. Hieu ro co che dinh thoi voi Timeout (`K_FOREVER`, `K_NO_WAIT`, `K_MSEC`).

---

## 2. Phan bo chan phan cung (Pinout Configuration)

| Ngoai vi / Tin hieu | Chan GPIO STM32F407 | Alias Devicetree | Che do hoat dong |
| :--- | :--- | :--- | :--- |
| LED3 (DEBUG LED) | `PE4` | `led0` | GPIO Output, Active-High |
| Output Y0 | `PE5` | `led1` | GPIO Output, Active-High |
| Output Y1 | `PE6` | `led2` | GPIO Output, Active-High |
| Role cong suat (Relay) | `PE2` | `relay0` (hoac direct GPIO) | GPIO Output, Active-High |
| Nut nhan ngoai (KEY) | `PB12` | `sw0` (hoac direct GPIO) | GPIO Input, Interrupt Edge Falling, Pull-up |
| Console Debug | `PA9` / `PA10` | `usart1` | Baudrate 115200 bps, 8-N-1 |

---

## 3. Danh muc cac bai thuc hanh va Mo ta chi tiet

### 3.1. Cac bai Demo nen tang (`demos/`)

- `lab2_demo1_binary_sem`:
  - K_timer tao ngat moi 2 giay.
  - Ham ngat chi thuc hien `k_sem_give(&evt_sem)`.
  - Luong thuc thi cho `k_sem_take(&evt_sem, K_FOREVER)` va nhay LED3 3 lan.
  - Minh hoa ky thuat xu ly ngat tri hoan (Deferred Interrupt Processing).
- `lab2_demo2_counting_sem`:
  - Timer tao ra 15 su kien don dap (burst, cach nhau 300 ms), trong khi luong chi xu ly duoc voi toc do 1000 ms/su kien.
  - Counting Semaphore voi gioi han `EVT_LIMIT = 5` dong vai tro bo dem nho cac su kien chua duoc xu ly.
- `lab2_demo3_mutex`:
  - 2 luong cung chay vong lap 1000 lan de tang mot bien dem chung `counter`.
  - Khi `USE_MUTEX = 0`: Xay ra hien tuong xung dot tai nguyen (Race Condition), ket qua bien dem khong dat 2000.
  - Khi `USE_MUTEX = 1`: `k_mutex_lock()` va `k_mutex_unlock()` dam bao tinh toan ven du lieu, ket qua luon chinh xac 2000.
- `lab2_demo4_msgq`:
  - Luong Producer dong goi cau truc `struct led_cmd` (chua chi so den va thoi gian sang) va gui vao hang doi qua `k_msgq_put()`.
  - Luong Consumer doc lenh qua `k_msgq_get()` va dieu khien den theo thu tu vao truoc ra truoc (FIFO).

---

### 3.2. Cac bai tap ung dung (`lab2_ex*`)

#### Bai 1: `lab2_ex1_relay_binary_sem` - Dieu khien Role bao ve qua nhiet
- **Yeu cau:** Mo phong he thong an toan cong nghiep.
- **Thiet ke:**
  - Luong 1 (Sensor Monitor): Doc cam bien gia lap. Khi phat hien nhiet do vuot nguong an toan (> 50°C), lap tuc phat tin hieu `k_sem_give(&overheat_sem)`.
  - Luong 2 (Safety Protection Thread): Bi block o `k_sem_take(&overheat_sem, K_FOREVER)`. Ngay khi nhan duoc tin hieu, lap tuc ngat Role (`PE2`) de cat nguon tai, dong thoi kich hoat den canh bao va in log khan cap ra UART.

#### Bai 2: `lab2_ex2_timeout_monitor` - Giam sat loi Timeout cam bien
- **Yeu cau:** Phat hien su co cam bien bi treo hoac mat ket noi bang co che Semaphore timeout.
- **Thiet ke:**
  - Luong Sensor: Dinh ky gui tin hieu song con (Heartbeat Ping) qua Semaphore.
  - Luong Monitor: Goi `k_sem_take(&sensor_sem, K_MSEC(2500))`.
  - Neu cam bien khong phan hoi trong vong 2.5 giay, ham tra ve `-EAGAIN` (Timeout). Luong Monitor xac dinh cam bien da chet, kich hoat che do canh bao loi he thong.

#### Bai 3: `lab2_ex3_counting_sem_workers` - Quan ly Resource Pool bang Counting Semaphore
- **Yeu cau:** Xay dung mo hinh Pool gom 3 Worker Threads thuc thi cac tac vu tinh toan doc lap.
- **Thiet ke:**
  - Khoi tao `K_SEM_DEFINE(worker_pool_sem, 3, 3)`.
  - Luong Dispatcher khi nhan cong viec moi se kiem tra tinh san sang cua pool bang cach `k_sem_take(&worker_pool_sem, K_NO_WAIT)`.
  - Neu pool con tai nguyen, cong viec duoc phan phoi; khi worker hoan tat nhiem vu se goi `k_sem_give(&worker_pool_sem)` de tra lai quyen phuc vu.

#### Bai 4: `lab2_ex4_isr_message_queue` - Giao tiep ISR toi Thread qua Message Queue
- **Yeu cau:** Xu ly ngat nut nhan phan cung khong gay tre he thong.
- **Thiet ke:**
  - Cau hinh ngat GPIO Falling Edge tren chan nut bam `PB12`.
  - Trong ham callback ngat `button_pressed_isr()`:
    - Tuyet doi khong dung `k_msleep` hoac in chuoi dai.
    - Dong goi su kien `struct btn_event` gom timestamp va trang thai.
    - Goi `k_msgq_put(&btn_msgq, &evt, K_NO_WAIT)`. Neu hang doi day, su kien bi bo qua ma khong gay treo MCU.
  - Luong Worker: Cho tin hieu tu hang doi bang `k_msgq_get(&btn_msgq, &evt, K_FOREVER)` de thuc hien xu ly logic chuyen doi che do hoat dong.

#### Bai 5: `lab2_ex5_pedestrian_traffic_light` - Den giao thong uu tien nguoi di bo
- **Yeu cau:** He thong den giao thong hoat dong binh thuong theo chu ky FSM (Do 7s, Xanh 5s, Vang 2s). Khi co nguoi di bo bam nut yeu cau qua duong (`PB12`), he thong phai uu tien chuyen pha an toan.
- **Thiet ke:**
  - Ngat nut bam phan cung dua su kien vao Message Queue hoac Binary Semaphore.
  - Luong FSM dinh ky kiem tra su kien qua doan thoi gian nho:
    - Khi phat hien yeu cau qua duong, chu trinh Xanh bi rut ngan ve Vang va lap tuc chuyen sang Do cho phuong tien giao thong.
    - Kich hoat den uu tien nguoi di bo trong 5 giay.
    - Sau khi hoan tat pha uu tien, he thong tu dong phuc hoi chu trinh binh thuong.

---

## 4. Huong dan Bien dich va Nap chuong trinh

Tu thu muc goc cua nhanh `lab2_synchronization`:

```bash
# Bien dich Bai 4: ISR sang Thread qua Message Queue
west build -p always -b stm32f4_disco lab2_ex4_isr_message_queue

# Nap vao kit BKIT ARM4
west flash

# Giam sat ket qua tren Serial Console
picocom -b 115200 /dev/ttyUSB0
```

---

## 5. Quy tac ky thuat va Luu y can nho

1. **Quy tac bat di bat dich trong ISR:**
   - Khong bao gio duoc goi `k_mutex_lock()` ben trong ham ngat ISR (se gay loi kernel panic vi ISR khong the bi block).
   - Khong duoc goi `k_sem_take()` voi timeout khac `K_NO_WAIT`.
   - Ham `k_msgq_put()` trong ISR chi duoc su dung timeout la `K_NO_WAIT`.
2. **Priority Inversion va Mutex:** Khi hai luong co do uu tien chenh lech cung truy cap vao tai nguyen chung, luon su dung `k_mutex` thay vi `k_sem` de Zephyr tu dong ap dung co che Priority Inheritance ngan chan hien tuong dao nguoc do uu tien.
