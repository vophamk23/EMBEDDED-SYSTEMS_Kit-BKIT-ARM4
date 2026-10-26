# HE THONG NHUNG (EMBEDDED SYSTEMS) - ZEPHYR RTOS APPLICATIONS

Khoa Khoa hoc va Ky thuat May tinh - Truong Dai hoc Bach Khoa, DHQG-HCM  
Mon hoc: He thong nhung (CO3037 / CO3038)  
Nen tang phan cung: Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
He dieu hanh thoi gian thuc: Zephyr RTOS v4.4.0  

---

## 1. Tong quan du an

Repository nay chua toan bo ma nguon, cau hinh Devicetree, Kconfig va tai lieu huong dan thuc hanh mon hoc He thong nhung. Du an duoc thiet ke dua tren he dieu hanh thoi gian thuc ma nguon mo cong nghiep Zephyr RTOS, tap trung vao cac ky nang:
- Lap trinh he thong da luong (Multithreading) va lap lich thoi gian thuc (Real-time Scheduling).
- Dong bo tien trinh va truyen thong lien tien trinh (Synchronization & IPC: Semaphores, Mutex, Message Queue).
- Mang truyen thong vi dieu khien cong nghiep Controller Area Network (CAN Bus) va he thong phan tan.
- Tich hop he thong nhung phuc tap trong do an cuoi ky (Capstone Assignment).

---

## 2. Cau truc cac nhanh (Git Branching Strategy)

He thong ma nguon duoc to chuc thanh cac nhanh doc lap phuc vu cho tung giai doan dao tao va nghien cuu:

| Ten nhanh | Noi dung chuyen biet | Trinh do / Muc tieu |
| :--- | :--- | :--- |
| `main` | Nhanh tong hop chua toan bo ma nguon, tai lieu va ma tran lab cua khoa hoc. | Tham chieu chung toan khoa |
| `lab1_multithreading` | Co ban ve Zephyr RTOS, quan ly da luong, dinh thoi, uu tien thuc thi. | Nen tang RTOS & Da nhiem |
| `lab2_synchronization` | Dong bo tien trinh, Semaphore, Mutex ngan ngua race condition, Message Queue, Deferred ISR. | Dong bo & Truyen thong IPC |
| `lab3_canbus` | Giao thuc CAN 2.0A/B, bo loc phan cung (Hardware Filters), khung RTR, mang 2 node phan tan. | Mang nhung & He phan tan |
| `assignment` | Khung kien truc do an cuoi ky tich hop da luong, IPC, CAN Bus va ngoai vi. | Do an tong hop (Capstone) |

---

## 3. Dac ta phan cung Kit BKIT ARM4

He thong su dung board phat trien BKIT ARM4 duoc thiet ke boi DH Bach Khoa TP.HCM cho muc dich dao tao ky su nhung:

### 3.1. Thong so ky thuat vi dieu khien
- Vi dieu khien: STM32F407ZGT6 (STMicroelectronics).
- Loi xu ly: ARM 32-bit Cortex-M4 voi FPU (Floating Point Unit) va tap lenh DSP.
- Tan so hoat dong: 168 MHz (HSE 8 MHz thach anh ngoai).
- Bo nho Flash: 1024 KB (1 MB).
- Bo nho SRAM: 192 KB (128 KB SRAM1/SRAM2 + 64 KB CCM SRAM).

### 3.2. So do phan bo chan ngoai vi (Pinout Mapping)
Cac chan ngoai vi tren kit duoc cau hinh dong nhat thong qua file `app.overlay` trong cac du an:

| Chuc nang | Ten tren Board | Chan GPIO STM32F407 | Che do / Muc logic |
| :--- | :--- | :--- | :--- |
| LED He thong | LED DEBUG (LED3) | `PE4` | Output, Active High (`led0`) |
| Ngo ra dieu khien 0 | Output Y0 | `PE5` | Output, Active High (`led1`) |
| Ngo ra dieu khien 1 | Output Y1 | `PE6` | Output, Active High (`led2`) |
| Role cong suat | RELAY IN1 | `PE2` | Output, Active High |
| Nut nhan ngoai | KEY / BTN | `PB12` | Input, Active Low (Pull-up) |
| CAN1 Receive | CAN1_RX | `PB8` | Alternate Function 9 (AF9) |
| CAN1 Transmit | CAN1_TX | `PB9` | Alternate Function 9 (AF9) |
| Console Debug | USART1_TX / RX | `PA9` / `PA10` | 115200 bps, 8-N-1 (qua MAX232 DB9) |

---

## 4. Yeu cau moi truong va Cai dat

### 4.1. Cong cu can thiet
- He dieu hanh: Ubuntu Linux 22.04 LTS / 24.04 LTS hoac Windows 10/11 (WSL2 / PowerShell).
- Trinh quan ly du an Zephyr: `west` (phien ban 1.2.0 tro len).
- Bo cong cu bien dich: Zephyr SDK (v0.16.x tro len) bao gom trinh bien dich `arm-zephyr-eabi-gcc`.
- Trinh nap chuong trinh: OpenOCD / ST-Link CLI.
- Phan mem giam sat UART: PuTTY, Tera Term, hoac Serial Monitor trong VS Code (Baudrate: 115200).

### 4.2. Khoi tao workspace Zephyr
Neu chua co san workspace Zephyr tren may, thuc hien theo quy trinh chuan:

```bash
# Tao moi truong ao Python
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate    # Tren Windows: .venv\Scripts\activate

# Cai dat west
pip install west

# Khoi tao ma nguon Zephyr v4.4.0
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0 ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
```

---

## 5. Huong dan su dung Repository

### 5.1. Clone ma nguon
```bash
git clone https://github.com/vophamk23/EMBEDDED-SYSTEMS.git
cd EMBEDDED-SYSTEMS
```

### 5.2. Chuyen doi qua lai giua cac nhanh
De lam viec voi tung bai thuc hanh, chuyen nhanh tuong ung bang Git:

```bash
# Xem danh sach cac nhanh tren remote
git branch -a

# Chuyen sang Lab 1: Da luong
git checkout lab1_multithreading

# Chuyen sang Lab 2: Dong bo va IPC
git checkout lab2_synchronization

# Chuyen sang Lab 3: Mang CAN Bus
git checkout lab3_canbus

# Chuyen sang Assignment: Do an cuoi ky
git checkout assignment
```

### 5.3. Quy trinh bien dich va nap chuong trinh (Build & Flash)
Moi thu muc bai tap (vi du: `lab1_ex1_asym_blinky`) la mot ung dung Zephyr doc lap. Thuc hien lenh sau tu thu muc goc cua nhanh:

```bash
# Bien dich ung dung (su dung board stm32f4_disco duoc overlay boi app.overlay)
west build -p always -b stm32f4_disco path/to/project_dir

# Nap code vao vi dieu khien qua ST-Link
west flash

# Xoa bo nho cache va bien dich lai tu dau
west build -p pristine -b stm32f4_disco path/to/project_dir
```

---

## 6. Danh muc tai lieu tham khao

1. Zephyr Project Documentation (v4.4.0): https://docs.zephyrproject.org/latest/
2. STM32F407ZGT6 Reference Manual (RM0090): STMicroelectronics.
3. Cortex-M4 Generic User Guide: ARM Holdings.
4. Giao trinh Thuc hanh He thong nhung - Khoa KH&KT May tinh, DH Bach Khoa TP.HCM.
