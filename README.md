# 🧵 LAB 1: ĐA LUỒNG VÀ LẬP LỊCH THỜI GIAN THỰC TRONG ZEPHYR RTOS
### Hướng dẫn cài đặt môi trường, Quản lý luồng tĩnh & động, Phân cấp độ ưu tiên & FSM

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (Embedded Systems)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `lab1_multithreading` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Mục tiêu thí nghiệm](#1-mục-tiêu-thí-nghiệm)
- [2. Hướng dẫn toàn diện cài đặt môi trường phát triển (Phần A)](#2-hướng-dẫn-toàn-diện-cài-đặt-môi-trường-phát-triển-phần-a)
  - [Bước A1: Cài đặt Visual Studio Code](#bước-a1-cài-đặt-visual-studio-code)
  - [Bước A2: Cài các công cụ nền bằng Windows Package Manager (winget)](#bước-a2-cài-các-công-cụ-nền-bằng-windows-package-manager-winget)
  - [Bước A3: Tạo môi trường ảo Python (venv) và cài west](#bước-a3-tạo-môi-trường-ảo-python-venv-và-cài-west)
  - [Bước A4: Tải mã nguồn Zephyr v4.4.0](#bước-a4-tải-mã-nguồn-zephyr-v440)
  - [Bước A5: Đăng ký Zephyr với CMake & Cài thư viện phụ thuộc](#bước-a5-đăng-ký-zephyr-với-cmake--cài-thư-viện-phụ-thuộc)
  - [Bước A6: Cài đặt Zephyr SDK Toolchain (ARM Embedded)](#bước-a6-cài-đặt-zephyr-sdk-toolchain-arm-embedded)
  - [Bước A7: Cài driver mạch nạp ST-LINK & STM32CubeProgrammer](#bước-a7-cài-driver-mạch-nạp-st-link--stm32cubeprogrammer)
  - [Bước A8: Cài đặt Extension cho VS Code](#bước-a8-cài-đặt-extension-cho-vs-code)
  - [Bước A9: Khởi tạo thư mục ứng dụng Workspace](#bước-a9-khởi-tạo-thư-mục-ứng-dụng-workspace)
- [3. Chuẩn bị Kit BKIT ARM4 & Cấu hình Devicetree](#3-chuẩn-bị-kit-bkit-arm4--cấu-hình-devicetree)
- [4. Cấu trúc thư mục dự án](#4-cấu-trúc-thư-mục-dự-án)
- [5. Danh mục các bài thực hành & bài tập chi tiết](#5-danh-mục-các-bài-thực-hành--bài-tập-chi-tiết)
  - [Bài mẫu 0: bkit_blinky - Kiểm tra phần cứng LED3](#bài-mẫu-0-bkit_blinky---kiểm-tra-phần-cứng-led3)
  - [Bài 1: lab1_ex1_asym_blinky - Nháy LED bất đối xứng](#bài-1-lab1_ex1_asym_blinky---nháy-led-bất-đối-xứng)
  - [Bài 2: lab1_ex2_sync_led_y0 - Đồng bộ qua biến dùng chung](#bài-2-lab1_ex2_sync_led_y0---đồng-bộ-qua-biến-dùng-chung)
  - [Bài 3: lab1_ex3_independent_patterns - Mẫu nháy đa luồng độc lập lệch pha](#bài-3-lab1_ex3_independent_patterns---mẫu-nháy-đa-luồng-độc-lập-lệch-pha)
  - [Bài 4: lab1_ex4_multi_led_param - Tái sử dụng Thread qua con trỏ cấu hình](#bài-4-lab1_ex4_multi_led_param---tái-sử-dụng-thread-qua-con-trỏ-cấu-hình)
  - [Bài 5: lab1_ex5_traffic_light - Hệ thống đèn giao thông FSM kết hợp Heartbeat](#bài-5-lab1_ex5_traffic_light---hệ-thống-đèn-giao-thông-fsm-kết-hợp-heartbeat)
- [6. Hướng dẫn biên dịch & Nạp chương trình](#6-hướng-dẫn-biên-dịch--nạp-chương-trình)
- [7. So sánh tư duy kiến trúc: Zephyr RTOS vs FreeRTOS / Arduino](#7-so-sánh-tư-duy-kiến-trúc-zephyr-rtos-vs-freertos--arduino)

---

## 🎯 1. Mục tiêu thí nghiệm

1. **Thiết lập hoàn chỉnh môi trường phát triển:** Cài đặt và cấu hình toolchain Zephyr RTOS v4.4.0, Zephyr SDK, công cụ quản trị `west` và bộ nạp `ST-LINK` trên nền tảng VS Code.
2. **Làm chủ kiến trúc đa luồng (Multithreading):** Hiểu rõ cách Kernel Zephyr phân chia thời gian thực thi CPU giữa các luồng độc lập, loại bỏ triệt để các hàm chờ bận (`busy-wait`) tiêu tốn năng lượng.
3. **Phân cấp độ ưu tiên luồng (Thread Priority):**
   - **Cooperative Threads (Độ ưu tiên âm: `-15` đến `-1`):** Luồng hợp tác, không bị chiếm quyền bởi bất kỳ luồng nào trừ khi tự nguyện nhường CPU (`k_yield()`, `k_msleep()`).
   - **Preemptive Threads (Độ ưu tiên không âm: `0` đến `14`):** Luồng chiếm quyền, sẵn sàng bị tạm dừng khi có luồng độ ưu tiên cao hơn (giá trị số nhỏ hơn) thức giấc.
4. **Cơ chế Devicetree và Hardware Abstraction:** Ánh xạ phần cứng qua file `app.overlay` để tách biệt mã nguồn C khỏi sự phụ thuộc chân vi điều khiển cụ thể.

---

## 🛠️ 2. Hướng dẫn toàn diện cài đặt môi trường phát triển (Phần A)

> [!NOTE]
> Phần cài đặt này chỉ cần thực hiện **một lần duy nhất** trên máy tính phát triển (Windows 10/11 64-bit).

### Bước A1: Cài đặt Visual Studio Code
- Tải và cài đặt VS Code từ trang chủ: [code.visualstudio.com](https://code.visualstudio.com/).
- Trong quá trình cài, tích chọn **Add to PATH** để có thể gọi lệnh `code` từ cửa sổ dòng lệnh.

### Bước A2: Cài các công cụ nền bằng Windows Package Manager (winget)
Mở terminal PowerShell (với quyền Administrator) và thực thi lệnh sau để tự động cài đặt toàn bộ công cụ nền tảng:

```powershell
winget install Kitware.CMake Ninja-build.Ninja oss-winget.gperf Python.Python.3.12 Git.Git oss-winget.dtc wget 7zip.7zip
```

> [!IMPORTANT]
> **Xử lý xung đột App execution aliases của Windows:**
> Mặc định Windows trỏ lệnh `python` tới Microsoft Store. Hãy vào **Windows Settings** $\rightarrow$ **Apps** $\rightarrow$ **Advanced app settings** $\rightarrow$ **App execution aliases**, tìm và chuyển **TẮT (OFF)** hai mục `python.exe` và `python3.exe`.
> *Yêu cầu phiên bản Python:* Phải sử dụng Python 3.10, 3.11 hoặc 3.12.

### Bước A3: Tạo môi trường ảo Python (venv) và cài west
Tạo thư mục làm việc chuẩn và thiết lập môi trường Python ảo cô lập nhằm tránh xung đột gói:

```powershell
# Chuyển về thư mục người dùng cá nhân
cd $HOME

# Tạo thư mục gốc chứa toàn bộ hệ sinh thái Zephyr
mkdir zephyrproject
cd zephyrproject

# Khởi tạo môi trường ảo Python
python -m venv .venv

# Kích hoạt môi trường ảo
.venv\Scripts\Activate.ps1

# Cài đặt công cụ quản trị Zephyr Meta-tool (west)
pip install west
```

### Bước A4: Tải mã nguồn Zephyr v4.4.0
Sử dụng công cụ `west` để tải chính xác phiên bản mã nguồn Zephyr v4.4.0 cùng toàn bộ các kho mô-đun phần cứng liên quan (HAL STM32, CMSIS...):

```powershell
# Khởi tạo workspace trỏ đến Zephyr v4.4.0
west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0

# Đồng bộ tải toàn bộ module và kho lưu trữ phụ thuộc (dung lượng ~2-3 GB)
west update
```

### Bước A5: Đăng ký Zephyr với CMake & Cài thư viện phụ thuộc
Đăng ký vị trí gói Zephyr vào hệ thống CMake và cài đặt các thư viện Python bổ trợ cho quá trình sinh cấu hình Kconfig/Devicetree:

```powershell
# Xuất cấu hình Zephyr package cho CMake
west zephyr-export

# Cài đặt các gói thư viện Python phục vụ công cụ sinh code của Zephyr
pip install -r ..\zephyr\scripts\requirements.txt
```

### Bước A6: Cài đặt Zephyr SDK Toolchain (ARM Embedded)
Zephyr SDK cung cấp bộ công cụ biên dịch chéo (`arm-zephyr-eabi-gcc`) đồng bộ với kernel:

```powershell
# Tải và giải nén Zephyr SDK v1.0.1 (chỉ cần toolchain ARM Cortex-M)
cd $HOME
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v1.0.1/zephyr-sdk-1.0.1_windows-x86_64_arm-zephyr-eabi.7z -OutFile zephyr-sdk.7z
7z x zephyr-sdk.7z -o"$HOME"
Remove-Item zephyr-sdk.7z

# Đăng ký vị trí Toolchain với hệ điều hành
cd $HOME\zephyr-sdk-1.0.1
.\setup.cmd
```

### Bước A7: Cài driver mạch nạp ST-LINK & STM32CubeProgrammer
1. **Driver ST-LINK (STSW-LINK009):** Tải gói driver ST-LINK từ trang web của STMicroelectronics, chạy file `stlink_winusb_install.bat` với quyền **Run as Administrator**. Khi kết nối kit qua cổng ST-LINK, kiểm tra trong **Device Manager** xuất hiện mục:
   `Universal Serial Bus devices` $\rightarrow$ `STM32 STLink`.
2. **STM32CubeProgrammer:** Tải và cài đặt phần mềm [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) để hỗ trợ nạp firmware tự động qua `west flash`.

### Bước A8: Cài đặt Extension cho VS Code
Mở VS Code và cài đặt các tiện ích mở rộng quan trọng:
- **C/C++** (`ms-vscode.cpptools`)
- **CMake Tools** (`ms-vscode.cmake-tools`)
- **Serial Monitor** (`ms-vscode.vscode-serial-monitor`)

### Bước A9: Khởi tạo thư mục ứng dụng Workspace
Mọi bài thực hành được đặt bên trong thư mục `zephyrproject\applications`:
```powershell
cd $HOME\zephyrproject
mkdir applications
cd applications
```

---

## 🔌 3. Chuẩn bị Kit BKIT ARM4 & Cấu hình Devicetree

### 3.1. Kết nối phần cứng Kit BKIT ARM4
- Cấp nguồn Adapter 12V DC vào jack nguồn chính của Kit.
- Gạt công tắc nguồn chính **SW11** sang vị trí **ON** (Đèn LED nguồn đỏ sáng).
- Cắm cáp USB-C từ máy tính vào cổng **ST-LINK** trên kit để vừa nạp vừa xem log UART (USART1).

### 3.2. Bảng ánh xạ ngoại vi qua Devicetree (`app.overlay`)
Hệ thống sử dụng cơ chế Devicetree để trừu tượng hóa phần cứng, tránh viết trực tiếp tên thanh ghi phần cứng trong code:

| Tên thiết bị trên Kit | Chân MCU STM32F407 | Node Label Devicetree | Alias tiêu chuẩn | Trạng thái tích cực |
| :--- | :--- | :--- | :--- | :--- |
| **LED3 (DEBUG LED)** | `PE4` | `green_led_3` | `led0` | Active-High (`GPIO_ACTIVE_HIGH`) |
| **Output Y0** | `PE5` | `output_y0` | `led1` | Active-High (`GPIO_ACTIVE_HIGH`) |
| **Output Y1** | `PE6` | `output_y1` | `led2` | Active-High (`GPIO_ACTIVE_HIGH`) |

Nội dung trích đoạn file `app.overlay`:
```dts
/ {
    aliases {
        led0 = &green_led_3;
        led1 = &output_y0;
        led2 = &output_y1;
    };

    leds {
        compatible = "gpio-leds";
        green_led_3: led_3 {
            gpios = <&gpioe 4 GPIO_ACTIVE_HIGH>;
            label = "User LED3";
        };
        output_y0: y_0 {
            gpios = <&gpioe 5 GPIO_ACTIVE_HIGH>;
            label = "Output Y0";
        };
        output_y1: y_1 {
            gpios = <&gpioe 6 GPIO_ACTIVE_HIGH>;
            label = "Output Y1";
        };
    };
};
```

---

## 📁 4. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab1_multithreading)
├── bkit_blinky/                        # Bài mẫu 0: Kiểm tra phần cứng LED3
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── app.overlay
│   └── src/main.c
├── lab1_ex1_asym_blinky/               # Bài 1: Luồng nháy LED bất đối xứng
├── lab1_ex2_sync_led_y0/               # Bài 2: Đồng bộ 2 luồng qua biến volatile
├── lab1_ex3_independent_patterns/      # Bài 3: Hai luồng chạy chu kỳ độc lập lệch pha
├── lab1_ex4_multi_led_param/           # Bài 4: Tái sử dụng hàm Thread qua tham số
├── lab1_ex5_traffic_light/             # Bài 5: FSM đèn giao thông kết hợp Heartbeat
├── LAB1_MANUAL_Zephyr_BKIT_ARM4.docx   # File tài liệu hướng dẫn thực hành gốc
├── LAB1_EXERCISE_Zephyr_BKIT_ARM4.docx # File phiếu bài tập và mẫu nộp bài
├── .gitignore
└── README.md                           # Tài liệu kỹ thuật chi tiết Lab 1
```

---

## 📝 5. Danh mục các bài thực hành & bài tập chi tiết

### Bài mẫu 0: `bkit_blinky` - Kiểm tra phần cứng LED3
* **Mục tiêu:** Kiểm tra đường nạp firmware và độ thông mạch của LED3 trên chân `PE4`.
* **Cơ chế:** Tạo luồng đơn giản, nháy LED đối xứng chu kỳ 1s (500ms Sáng / 500ms Tắt) bằng `gpio_pin_toggle_dt(&led3)`.

---

### Bài 1: `lab1_ex1_asym_blinky` - Nháy LED bất đối xứng
* **Mục tiêu:** Cấu hình luồng nháy bất đối xứng thông qua hàm định thời không chặn `k_msleep()`.
* **Thông số chu kỳ:**
  * Thời gian SÁNG (ON): **1000 ms** (1 giây).
  * Thời gian TẮT (OFF): **2000 ms** (2 giây).
  * Chu kỳ tổng: **3000 ms** (3 giây), lặp vô hạn.
* **Khai báo Thread:**
  ```c
  K_THREAD_DEFINE(blinky_thread, 1024, blinky_entry, NULL, NULL, NULL, 7, 0, 0);
  ```
* **Kiểm chứng:** Đo đạc chính xác bằng logic analyzer hoặc quan sát LED3: đèn sáng 1 giây rồi tắt 2 giây đều đặn.

---

### Bài 2: `lab1_ex2_sync_led_y0` - Đồng bộ qua biến dùng chung
* **Mục tiêu:** Đồng bộ trạng thái giữa 2 luồng độc lập bằng biến cờ toàn cục.
* **Cơ chế:**
  * **Master Thread (`thread_led_blinky`):** Chịu trách nhiệm nháy LED3 (sáng 1s / tắt 1s) và cập nhật trạng thái vào biến toàn cục:
    ```c
    volatile bool g_led_state;
    ```
  * **Follower Thread (`thread_y0_follow`):** Thăm dò biến `g_led_state` định kỳ mỗi 10 ms (`k_msleep(10)`). Khi phát hiện trạng thái thay đổi, lập tức cập nhật ngõ ra Y0 (`PE5`) sáng/tắt tương ứng theo LED3.
  * Từ khóa `volatile` bắt buộc phải có để báo cho trình biên dịch GCC không tối ưu hóa biến vào thanh ghi CPU mà luôn đọc trực tiếp từ bộ nhớ RAM.

---

### Bài 3: `lab1_ex3_independent_patterns` - Mẫu nháy đa luồng độc lập lệch pha
* **Mục tiêu:** Vận hành hai luồng chạy song song hoàn toàn độc lập với chu kỳ và pha khác nhau.
* **Thiết kế:**
  * **Thread 1 (LED3):** Nháy tuần hoàn chu kỳ 2s (1000ms BẬT / 1000ms TẮT).
  * **Thread 2 (Output Y0 & Y1):** Điều khiển 2 ngõ ra tuần tự theo chu kỳ 5s gồm 3 pha:
    * *Pha 1 (2000 ms):* Y0 BẬT, Y1 TẮT.
    * *Pha 2 (2000 ms):* Y0 TẮT, Y1 BẬT.
    * *Pha 3 (1000 ms):* Cả Y0 và Y1 cùng TẮT.
* **Nguyên lý:** Nhờ bộ điều phối Zephyr RTOS, khi một luồng gọi `k_msleep()`, CPU lập tức chuyển sang phục vụ luồng kia mà không bị gián đoạn hay sai lệch thời gian.

---

### Bài 4: `lab1_ex4_multi_led_param` - Tái sử dụng Thread qua con trỏ cấu hình
* **Mục tiêu:** Tối ưu hóa bộ nhớ mã nguồn bằng cách tái sử dụng **một hàm Thread duy nhất** cho 3 đối tượng LED độc lập thông qua con trỏ tham số `p1`.
* **Cấu trúc dữ liệu:**
  ```c
  struct blink_cfg {
      const struct gpio_dt_spec *led;
      const char *name;
      uint32_t on_ms;
      uint32_t off_ms;
  };
  ```
* **Khởi tạo 3 Thread tĩnh độc lập:**
  ```c
  K_THREAD_DEFINE(t_led3, 1024, thread_blink, (void *)&cfg_led3, NULL, NULL, 7, 0, 0);
  K_THREAD_DEFINE(t_y0,   1024, thread_blink, (void *)&cfg_y0,   NULL, NULL, 7, 0, 0);
  K_THREAD_DEFINE(t_y1,   1024, thread_blink, (void *)&cfg_y1,   NULL, NULL, 7, 0, 0);
  ```

---

### Bài 5: `lab1_ex5_traffic_light` - Hệ thống đèn giao thông FSM kết hợp Heartbeat
* **Mục tiêu:** Xây dựng máy trạng thái hữu hạn (FSM) mô phỏng hệ thống đèn giao thông thực tế kết hợp luồng giám sát sống còn (Heartbeat).
* **Kiến trúc hệ thống:**
  * **Luồng Đèn Giao Thông (`thread_traffic_light` - Priority 5):**
    * *Pha ĐỎ (Y0 BẬT, Y1 TẮT):* Kéo dài **7 giây**.
    * *Pha XANH (Y0 TẮT, Y1 BẬT):* Kéo dài **5 giây**.
    * *Pha VÀNG (Cả Y0 và Y1 BẬT):* Kéo dài **2 giây**.
  * **Luồng Heartbeat (`thread_heartbeat` - Priority 7):**
    * Nháy LED3 độc lập chu kỳ 2s (1s BẬT / 1s TẮT) để báo hiệu vi điều khiển đang vận hành bình thường.
  * In log chi tiết từng giây qua UART console: `[   12000 ms] [TRAFFIC LIGHT] -> PHA XANH: 5 giay`.

---

## 💻 6. Hướng dẫn biên dịch & Nạp chương trình

Thực hiện lệnh với công cụ `west` từ thư mục gốc của dự án:

```powershell
# 1. Kích hoạt môi trường ảo Python
cd $HOME\zephyrproject
.venv\Scripts\Activate.ps1

# 2. Biên dịch một bài tập cụ thể (Ví dụ: Bài 5 Đèn giao thông)
west build -p auto -b stm32f4_disco applications\lab1\lab1_ex5_traffic_light

# 3. Nạp firmware xuống Kit BKIT ARM4
west flash

# 4. Mở Serial Monitor theo dõi log (115200 baud, 8-N-1)
# Hoặc sử dụng tính năng Serial Monitor tích hợp của VS Code
```

---

## ⚖️ 7. So sánh tư duy kiến trúc: Zephyr RTOS vs FreeRTOS / Arduino

| Tiêu chí | Arduino / Bare-metal | FreeRTOS (truyền thống) | Zephyr RTOS v4.4.0 |
| :--- | :--- | :--- | :--- |
| **Hàm chờ / Delay** | `delay()` gây treo CPU hoàn toàn | `vTaskDelay()` nhả CPU | `k_msleep()` đưa luồng vào trạng thái WAITING |
| **Mô tả phần cứng** | `pinMode(13, OUTPUT)` hardcode chân | Viết thanh ghi / STM32 HAL trực tiếp | **Devicetree (`app.overlay`)** chuẩn hóa phần cứng |
| **Cấu hình tính năng**| `#define` phân tán | `FreeRTOSConfig.h` thủ công | **Kconfig (`prj.conf`)** chuẩn Linux Kernel |
| **Khởi tạo luồng** | Không có (chỉ có vòng `loop()`) | `xTaskCreate()` cấp phát runtime | **`K_THREAD_DEFINE()`** tĩnh lúc biên dịch |
| **Quản lý dự án** | Arduino IDE đơn giản | Makefile / CubeIDE thủ công | **`west` meta-tool** quản trị đa repository |
