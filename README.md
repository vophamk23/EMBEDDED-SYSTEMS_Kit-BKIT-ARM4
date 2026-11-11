# 🔄 LAB 2: ĐỒNG BỘ TIẾN TRÌNH VÀ TRUYỀN THÔNG LIÊN TIẾN TRÌNH (IPC) TRONG ZEPHYR RTOS
### Lý thuyết toàn diện về Semaphore, Mutex Thừa kế ưu tiên, Message Queue, Quy tắc ISR & Hệ thống bài tập

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (Embedded Systems)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `lab2_synchronization` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Mục tiêu thí nghiệm](#1-mục-tiêu-thí-nghiệm)
- [2. Chuẩn bị phần cứng & Môi trường thực hành](#2-chuẩn-bị-phần-cứng--môi-trường-thực-hành)
- [3. Kiến thức nền tảng (Toàn diện theo Tài liệu hướng dẫn)](#3-kiến-thức-nền-tảng-toàn-diện-theo-tài-liệu-hướng-dẫn)
  - [3.1. Vì sao cần đồng bộ?](#31-vì-sao-cần-đồng-bộ)
  - [3.2. Semaphore — Binary và Counting](#32-semaphore--binary-và-counting)
  - [3.3. Mutex (Mutual Exclusion)](#33-mutex-mutual-exclusion)
  - [3.4. Message Queue (Hàng đợi thông điệp)](#34-message-queue-hàng-đợi-thông-điệp)
  - [3.5. Timeout và Giá trị trả về của Kernel](#35-timeout-và-giá-trị-trả-về-của-kernel)
  - [3.6. Quy tắc trong hàm ngắt (ISR)](#36-quy-tắc-trong-hàm-ngắt-isr)
  - [3.7. Cú pháp tương đương: Zephyr – FreeRTOS – CMSIS-RTOS2 (STM32Cube)](#37-cú-pháp-tương-đương-zephyr--freertos--cmsis-rtos2-stm32cube)
  - [3.8. Ba điểm dễ nhầm lẫn khi chuyển đổi giữa các RTOS](#38-ba-điểm-dễ-nhầm-lẫn-khi-chuyển-đổi-giữa-các-rtos)
- [4. Cấu trúc thư mục dự án](#4-cấu-trúc-thư-mục-dự-án)
- [5. Danh mục 4 bài Demo nền tảng (demos/)](#5-danh-mục-4-bài-demo-nền-tảng-demos)
  - [Demo 1: lab2_demo1_binary_sem - Xử lý ngắt hoãn lại (Deferred ISR)](#demo-1-lab2_demo1_binary_sem---xử-lý-ngắt-hoãn-lại-deferred-isr)
  - [Demo 2: lab2_demo2_counting_sem - Đệm sự kiện dồn dập (Burst Handling)](#demo-2-lab2_demo2_counting_sem---đệm-sự-kiện-dồn-dập-burst-handling)
  - [Demo 3: lab2_demo3_mutex - Khắc phục Race Condition bằng Mutex](#demo-3-lab2_demo3_mutex---khắc-phục-race-condition-bằng-mutex)
  - [Demo 4: lab2_demo4_msgq - Mô hình Producer - Consumer với Message Queue](#demo-4-lab2_demo4_msgq---mô-hình-producer---consumer-với-message-queue)
- [6. Danh mục 5 bài tập thực hành ứng dụng (lab2_ex*)](#6-danh-mục-5-bài-tập-thực-hành-ứng-dụng-lab2_ex)
  - [Bài 1: lab2_ex1_relay_binary_sem - Chạy tiếp sức ba đèn bằng Binary Semaphore](#bài-1-lab2_ex1_relay_binary_sem---chạy-tiếp-sức-ba-đèn-bằng-binary-semaphore)
  - [Bài 2: lab2_ex2_timeout_monitor - Giám sát tín hiệu ngắt có xử lý Timeout](#bài-2-lab2_ex2_timeout_monitor---giám-sát-tín-hiệu-ngắt-có-xử-lý-timeout)
  - [Bài 3: lab2_ex3_counting_sem_workers - Chia sẻ 2 kênh Output cho 4 Worker](#bài-3-lab2_ex3_counting_sem_workers---chia-sẻ-2-kênh-output-cho-4-worker)
  - [Bài 4: lab2_ex4_isr_message_queue - Thu thập dữ liệu từ ISR qua Message Queue](#bài-4-lab2_ex4_isr_message_queue---thu-thập-dữ-liệu-từ-isr-qua-message-queue)
  - [Bài 5: lab2_ex5_pedestrian_traffic_light - Đèn giao thông xin đường (Bài tổng hợp)](#bài-5-lab2_ex5_pedestrian_traffic_light---đèn-giao-thông-xin-đường-bài-tổng-hợp)
- [7. Hướng dẫn biên dịch & Nạp chương trình](#7-hướng-dẫn-biên-dịch--nạp-chương-trình)
- [8. Các lỗi kỹ thuật thường gặp & Cách khắc phục](#8-các-lỗi-kỹ-thuật-thường-gặp--cách-khắc-phục)

---

## 🎯 1. Mục tiêu thí nghiệm

Sau bài thực hành này, người học có thể:
1. **Phân biệt rõ ràng mục đích sử dụng:** Khi nào dùng binary semaphore, counting semaphore, mutex và message queue trong thiết kế hệ thống nhúng thời gian thực.
2. **Kỹ thuật xử lý ngắt an toàn:** Áp dụng thuần thục kỹ thuật *Deferred Interrupt Processing* (chuyển phần xử lý nặng từ hàm ngắt ISR sang thread thông qua semaphore hoặc message queue).
3. **Bảo vệ tài nguyên chia sẻ:** Sử dụng mutex để giải quyết triệt để lỗi tranh chấp dữ liệu (*Race Condition*) mà không gây ra hiện tượng đảo ngược độ ưu tiên (*Priority Inversion*).
4. **Xử lý Timeout chuyên nghiệp:** Vận dụng các giá trị timeout (`K_NO_WAIT`, `K_FOREVER`, `K_MSEC`) và kiểm tra chính xác mã lỗi trả về (`-EAGAIN`, `-EBUSY`, `-ENOMSG`).
5. **Chuyển đổi tư duy đa nền tảng:** Đối chiếu thông thạo cú pháp tương đương giữa Zephyr RTOS, FreeRTOS và CMSIS-RTOS2 (STM32CubeIDE).

---

## 🔌 2. Chuẩn bị phần cứng & Môi trường thực hành

- **Phần mềm:** Môi trường Zephyr RTOS v4.4.0, Zephyr SDK, công cụ `west` và VS Code (đã cài đặt ở Lab 1).
- **Phần cứng Kit BKIT ARM4:**
  - Nguồn 12V DC cắm vào jack nguồn chính.
  - Công tắc nguồn **SW11** gạt sang vị trí **ON**.
  - Cáp USB-C cắm vào cổng **ST-LINK** để nạp code và theo dõi Serial Monitor (115200 baud, 8-N-1 qua cổng USART1).
- **Cấu hình Devicetree (`app.overlay`):** Sử dụng 3 đèn LED trạng thái:
  - `led0` trỏ tới LED3 (`PE4` - User Debug LED)
  - `led1` trỏ tới Output Y0 (`PE5`)
  - `led2` trỏ tới Output Y1 (`PE6`)

---

## 📚 3. Kiến thức nền tảng (Toàn diện theo Tài liệu hướng dẫn)

### 3.1. Vì sao cần đồng bộ?
Ở Bài thực hành 1, mỗi thread điều khiển một đèn và chạy độc lập. Trong hệ thống thật, các thread phải phối hợp với nhau và chia sẻ tài nguyên. Có hai tình huống kinh điển:

1. **Chờ sự kiện:** Thread xử lý cần chờ ngắt từ nút nhấn, timer hay UART.
   - *Cách ngây thơ:* Dùng vòng lặp hỏi vòng `while (!flag) {}`. Hậu quả: CPU bị chiếm dụng 100% công suất mà không làm việc gì có ích, làm nóng vi điều khiển, các thread có cùng hoặc thấp hơn độ ưu tiên bị bỏ đói (*Starvation*).
   - *Cách đúng:* Cho thread chuyển sang trạng thái ngủ (**BLOCKED**) trên một đối tượng đồng bộ của hệ điều hành, giải phóng hoàn toàn chu kỳ CPU, và chỉ thức dậy khi sự kiện thực sự xảy ra.
2. **Truy cập dữ liệu dùng chung:** Câu lệnh C đơn giản `counter++` trong hợp ngữ thực chất là ba bước máy riêng biệt:
   $$\text{1. Đọc (Read RAM } \to \text{ Register)} \longrightarrow \text{2. Cộng (Modify Register + 1)} \longrightarrow \text{3. Ghi (Write Register } \to \text{ RAM)}$$
   Nếu bộ lập lịch chuyển sang thread khác giữa các bước đó, một hoặc nhiều lần tăng sẽ bị ghi đè và mất mát. Đoạn mã truy xuất dữ liệu chung đòi hỏi phải được thực thi trọn vẹn không bị gián đoạn như vậy gọi là **Vùng găng (Critical Section)**.

---

### 3.2. Semaphore — Binary và Counting

Semaphore là một bộ đếm số "thẻ bài" (token) nguyên không âm cùng với một danh sách các thread đang chờ. Trong Zephyr, một semaphore được khai báo bằng macro tĩnh:
```c
K_SEM_DEFINE(tên, giá_trị_đầu, giới_hạn);
```

#### Các hàm thao tác cốt lõi:
- **`k_sem_give(&s)`:** Tăng bộ đếm thêm 1 (nếu chưa đạt giới hạn), đánh thức một thread đang chờ trong hàng đợi. **Hàm này gọi được an toàn từ bên trong ISR.**
- **`k_sem_take(&s, timeout)`:** Nếu bộ đếm $> 0$, giảm 1 và trả về `0` ngay; nếu bộ đếm bằng 0, thread bị chặn (blocked) cho tới khi có token hoặc hết thời gian timeout.

#### Phân loại Semaphore:
* **Binary Semaphore (Giới hạn bằng 1):** Chỉ có hai trạng thái (0 hoặc 1), đại diện cho việc "đã có sự kiện" hay chưa. Rất phù hợp để báo hiệu sự kiện 1-1 từ ngắt (ISR) sang Thread (*Deferred Interrupt Processing*).
* **Counting Semaphore (Giới hạn $N > 1$):** Nhớ được tối đa $N$ sự kiện chưa kịp xử lý (bộ đệm tín hiệu), hoặc biểu diễn số lượng $N$ tài nguyên giống nhau đang còn trống trong một nhóm tài nguyên (*Resource Pool*).

---

### 3.3. Mutex (Mutual Exclusion)

Mutex bảo đảm tại mỗi thời điểm **chỉ một thread duy nhất** được phép bước vào vùng găng:
- Gọi `k_mutex_lock(&m, timeout)` ngay trước khi vào vùng găng.
- Gọi `k_mutex_unlock(&m)` ngay sau khi rời khỏi vùng găng.

#### Bảng so sánh chi tiết giữa Semaphore và Mutex:

| Đặc điểm | Semaphore (`k_sem`) | Mutex (`k_mutex`) |
| :--- | :--- | :--- |
| **Mục đích chính** | **Báo hiệu (Signaling) / Đếm sự kiện** | **Loại trừ lẫn nhau (Bảo vệ tài nguyên)** |
| **Chủ sở hữu (Ownership)** | **Không có chủ sở hữu:** Thread A take, Thread B (hoặc ISR) có thể give | **Có chủ sở hữu:** Chỉ duy nhất thread nào đã lock mới có quyền unlock |
| **Dùng trong ISR** | `k_sem_give` được; `k_sem_take` chỉ với `K_NO_WAIT` | **Tuyệt đối không được** (cả lock lẫn unlock) |
| **Lock nhiều lần** | Không áp dụng | **Hỗ trợ đệ quy:** Cùng một thread lock $N$ lần thì phải unlock đủ $N$ lần |
| **Đảo ngược ưu tiên** | **Không xử lý** (dễ dẫn đến Priority Inversion) | **Có kế thừa ưu tiên (Priority Inheritance)** giúp triệt tiêu Priority Inversion |

> [!IMPORTANT]
> **Hiện tượng Đảo ngược ưu tiên (Priority Inversion) và Cơ chế Kế thừa ưu tiên (Priority Inheritance):**
> Khi thread ưu tiên thấp (L) giữ khóa Mutex, thread ưu tiên cao (H) vào tranh chấp và bị chặn. Nếu một thread trung bình (M) xuất hiện, M chiếm CPU của L (vì ưu tiên $M > L$). Hậu quả là thread H bị gián tiếp đình trệ bởi thread M.
> **Mutex của Zephyr giải quyết việc này:** Kernel tự động tạm thời nâng độ ưu tiên của L lên bằng H. L chạy xong vùng găng và unlock Mutex ngay, nhường quyền chạy lại cho H, sau đó L trở về độ ưu tiên cũ.

---

### 3.4. Message Queue (Hàng đợi thông điệp)

Semaphore chỉ báo "có sự kiện" mà không mang theo nội dung. Khi cần gửi kèm dữ liệu (giá trị cảm biến, mã lệnh điều khiển, timestamp...):
```c
K_MSGQ_DEFINE(tên, kích_thước_phần_tử, số_phần_tử, căn_lề);
```
- Khởi tạo một bộ đệm vòng (Ring Buffer) cố định trong RAM.
- **`k_msgq_put(&q, &data, timeout)`:** Sao chép dữ liệu của phần tử vào cuối hàng đợi (theo giá trị). Do cơ chế copy bộ nhớ, biến cục bộ dùng để gửi có thể được tái sử dụng hoặc giải phóng ngay sau khi put.
- **`k_msgq_get(&q, &data, timeout)`:** Sao chép phần tử ở đầu hàng đợi ra ngoài theo thứ tự FIFO (First-In, First-Out).

---

### 3.5. Timeout và Giá trị trả về của Kernel

#### Bảng ý nghĩa các tham số Timeout:
| Tham số Timeout | Ý nghĩa vận hành |
| :--- | :--- |
| **`K_FOREVER`** | Chờ vô hạn cho tới khi thao tác thành công |
| **`K_NO_WAIT`** | Không chờ: thành công ngay hoặc trả về lỗi tức thì. **Bắt buộc khi gọi trong ISR** |
| **`K_MSEC(n)`, `K_SECONDS(n)`** | Chờ tối đa $n$ mili-giây / giây |

#### Bảng giá trị trả về của các hàm IPC:
| Hàm API | Thành công | `K_NO_WAIT` (Không có ngay) | Hết thời gian chờ (Timeout) |
| :--- | :---: | :---: | :---: |
| **`k_sem_take()`** | `0` | `-EBUSY` | `-EAGAIN` |
| **`k_mutex_lock()`** | `0` | `-EBUSY` | `-EAGAIN` |
| **`k_msgq_put()`** | `0` | `-ENOMSG` (Hàng đợi đầy) | `-EAGAIN` |
| **`k_msgq_get()`** | `0` | `-ENOMSG` (Hàng đợi rỗng) | `-EAGAIN` |

---

### 3.6. Quy tắc trong hàm ngắt (ISR)

Hàm ngắt (ISR) — bao gồm callback của `k_timer` và callback ngắt GPIO — chạy ngoài mọi thread, không có stack luồng riêng và **không bao giờ được phép chờ**.
* **Nguyên tắc chung:** ISR làm càng ít càng tốt, chỉ "báo hiệu" hoặc "gửi dữ liệu" rồi thoát ngay; phần tính toán nặng giao cho Thread đảm nhiệm (*Deferred Interrupt Processing*).

#### Bảng phân định API được phép / cấm gọi trong ISR:
| Nhóm hàm API | Được phép gọi trong ISR? | Ghi chú điều kiện |
| :--- | :---: | :--- |
| `k_sem_give()`, `k_sem_reset()`, `k_sem_count_get()` | **ĐƯỢC** | Thao tác tức thì, không gây chặn |
| `k_sem_take(&s, K_NO_WAIT)` | **ĐƯỢC** | **Chỉ với `K_NO_WAIT`** |
| `k_msgq_put(&q, &d, K_NO_WAIT)`, `k_msgq_get(&q, &d, K_NO_WAIT)` | **ĐƯỢC** | **Chỉ với `K_NO_WAIT`** |
| `k_timer_start()`, `k_timer_stop()` | **ĐƯỢC** | Quản lý bộ định thời phần mềm |
| `k_mutex_lock()`, `k_mutex_unlock()` | ❌ **CẤM** | Mutex yêu cầu Thread Ownership |
| `k_msleep()`, `k_sleep()`, `printf()` dài dòng | ❌ **CẤM** | Không ngủ trong ISR; tránh in ấn tốn thời gian |

---

### 3.7. Cú pháp tương đương: Zephyr – FreeRTOS – CMSIS-RTOS2 (STM32Cube)

Khi chuyển từ STM32CubeIDE sang Zephyr RTOS, bảng đối chiếu 14 thao tác chuẩn sau giúp chuyển đổi tư duy mã nguồn dễ dàng:

| Thao tác | Zephyr RTOS v4.4.0 | FreeRTOS (Chuẩn) | CMSIS-RTOS2 (STM32Cube) |
| :--- | :--- | :--- | :--- |
| **Tạo Binary Sem** | `K_SEM_DEFINE(s, 0, 1);` | `s = xSemaphoreCreateBinary();` | `s = osSemaphoreNew(1, 0, NULL);` |
| **Tạo Counting Sem**| `K_SEM_DEFINE(s, init, max);` | `s = xSemaphoreCreateCounting(max, init);` | `s = osSemaphoreNew(max, init, NULL);` |
| **Give (từ Thread)**| `k_sem_give(&s);` | `xSemaphoreGive(s);` | `osSemaphoreRelease(s);` |
| **Give (từ ISR)** | `k_sem_give(&s);` | `xSemaphoreGiveFromISR(s, &woken);`<br>`portYIELD_FROM_ISR(woken);` | `osSemaphoreRelease(s);` |
| **Take (Chờ mãi)** | `k_sem_take(&s, K_FOREVER);` | `xSemaphoreTake(s, portMAX_DELAY);` | `osSemaphoreAcquire(s, osWaitForever);` |
| **Take (Timeout 100ms)**| `k_sem_take(&s, K_MSEC(100));`| `xSemaphoreTake(s, pdMS_TO_TICKS(100));` | `osSemaphoreAcquire(s, 100);` *(tick)* |
| **Đọc bộ đếm** | `k_sem_count_get(&s);` | `uxSemaphoreGetCount(s);` | `osSemaphoreGetCount(s);` |
| **Tạo Mutex** | `K_MUTEX_DEFINE(m);` | `m = xSemaphoreCreateMutex();` | `m = osMutexNew(NULL);` |
| **Lock / Unlock** | `k_mutex_lock(&m, K_FOREVER);`<br>`k_mutex_unlock(&m);` | `xSemaphoreTake(m, portMAX_DELAY);`<br>`xSemaphoreGive(m);` | `osMutexAcquire(m, osWaitForever);`<br>`osMutexRelease(m);` |
| **Tạo Queue** | `K_MSGQ_DEFINE(q, sizeof(T), 8, 4);`| `q = xQueueCreate(8, sizeof(T));` | `q = osMessageQueueNew(8, sizeof(T), NULL);` |
| **Gửi (từ Thread)**| `k_msgq_put(&q, &d, K_FOREVER);` | `xQueueSend(q, &d, portMAX_DELAY);` | `osMessageQueuePut(q, &d, 0, osWaitForever);`|
| **Gửi (từ ISR)** | `k_msgq_put(&q, &d, K_NO_WAIT);` | `xQueueSendFromISR(q, &d, &woken);` | `osMessageQueuePut(q, &d, 0, 0);` |
| **Nhận** | `k_msgq_get(&q, &d, K_FOREVER);` | `xQueueReceive(q, &d, portMAX_DELAY);` | `osMessageQueueGet(q, &d, NULL, osWaitForever);`|
| **Đếm phần tử chờ**| `k_msgq_num_used_get(&q);` | `uxQueueMessagesWaiting(q);` | `osMessageQueueGetCount(q);` |

---

### 3.8. Ba điểm dễ nhầm lẫn khi chuyển đổi giữa các RTOS

> [!WARNING]
> 1. **Thứ tự tham số trong Counting Semaphore:**
>    * Trong **Zephyr**: thứ tự là `(tên, giá_trị_đầu, giới_hạn)` $\to$ `(s, init, max)`.
>    * Trong **FreeRTOS** và **CMSIS-RTOS2**: thứ tự bị đảo ngược là `(max, init)`!
> 2. **Hàm xử lý trong ISR:**
>    * **FreeRTOS** bắt buộc phải dùng các hàm riêng có hậu tố `...FromISR` và phải tự gọi macro chuyển ngữ cảnh `portYIELD_FROM_ISR(woken)`.
>    * **Zephyr** và **CMSIS-RTOS2** dùng chung một hàm cho cả Thread và ISR, chỉ cần truyền tham số timeout bằng `0` hoặc `K_NO_WAIT`.
> 3. **Đơn vị của Timeout:**
>    * **FreeRTOS / CMSIS-RTOS2** tính thời gian timeout theo số chu kỳ xung nhịp hệ điều hành (*Tick*), phụ thuộc vào tần số `configTICK_RATE_HZ`.
>    * **Zephyr** sử dụng trực tiếp các macro thời gian tường minh (`K_MSEC(n)`, `K_SECONDS(n)`), hoàn toàn độc lập với tần số tick bên dưới.
>
> *(Lưu ý: Zephyr cũng hỗ trợ lớp tương thích CMSIS-RTOS2 qua cấu hình `CONFIG_CMSIS_RTOS_V2=y` để biên dịch lại mã nguồn cũ của STM32Cube, tuy nhiên trong môn học khuyến khích sử dụng trực tiếp API gốc `k_...` của Zephyr).*

---

## 📁 4. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab2_synchronization)
├── demos/                              # 4 Bài Demo nghiên cứu các công cụ IPC
│   ├── lab2_demo1_binary_sem/          # Demo 1: Xử lý ngắt hoãn lại với Binary Semaphore
│   ├── lab2_demo2_counting_sem/        # Demo 2: Đệm sự kiện dồn dập với Counting Semaphore
│   ├── lab2_demo3_mutex/               # Demo 3: Xử lý Race Condition bằng Mutex
│   └── lab2_demo4_msgq/                # Demo 4: Mô hình Producer - Consumer với Message Queue
├── lab2_ex1_relay_binary_sem/          # Bài tập 1: Tiếp sức 3 đèn bằng 3 Binary Semaphore
├── lab2_ex2_timeout_monitor/           # Bài tập 2: Giám sát tín hiệu định kỳ có xử lý Timeout
├── lab2_ex3_counting_sem_workers/      # Bài tập 3: Chia sẻ 2 kênh Output cho 4 Worker
├── lab2_ex4_isr_message_queue/         # Bài tập 4: Thu thập dữ liệu từ ISR qua Message Queue
├── lab2_ex5_pedestrian_traffic_light/  # Bài tập 5: Đèn giao thông có nút xin qua đường tích hợp
├── LAB2_MANUAL_Zephyr_BKIT_ARM4.docx   # File tài liệu hướng dẫn thực hành gốc Lab 2
├── LAB2_EXERCISE_Zephyr_BKIT_ARM4.docx # File phiếu bài tập và mẫu nộp bài Lab 2
├── .gitignore
└── README.md                           # Tài liệu kỹ thuật toàn diện Lab 2
```

---

## 🔬 5. Danh mục 4 bài Demo nền tảng (`demos/`)

### Demo 1: `lab2_demo1_binary_sem` - Xử lý ngắt hoãn lại (Deferred ISR)
* **Mục tiêu:** Ứng dụng Binary Semaphore để chuyển tác vụ nặng từ ngắt sang luồng xử lý riêng biệt.
* **Cơ chế:** Timer hệ điều hành (`k_timer`) tạo ngắt định kỳ 2 giây. Trong ISR callback chỉ thực hiện `k_sem_give(&evt_sem)`. Luồng `thread_handler` ngủ trên `k_sem_take(&evt_sem, K_FOREVER)`, thức dậy nháy LED3 3 lần (tổng 600 ms). Ngắt thoát tức thì, không chiếm dụng CPU.

---

### Demo 2: `lab2_demo2_counting_sem` - Đệm sự kiện dồn dập (Burst Handling)
* **Mục tiêu:** Quan sát khả năng tích lũy sự kiện của Counting Semaphore khi nguồn phát nhanh hơn bộ phận xử lý.
* **Kịch bản Burst:** Timer bắn liên tiếp 15 sự kiện cách nhau 300 ms. Luồng Consumer xử lý mất 1000 ms/sự kiện. Semaphore khai báo `K_SEM_DEFINE(evt_count, 0, 5)`. 5 sự kiện đầu được lưu đệm, các sự kiện vượt quá bị mất mát (`lost`).

---

### Demo 3: `lab2_demo3_mutex` - Khắc phục Race Condition bằng Mutex
* **Mục tiêu:** Tái hiện hiện tượng tranh chấp dữ liệu (Race Condition) và chứng minh hiệu quả bảo vệ của Mutex.
* **Thực nghiệm:** Hai luồng `worker_a` và `worker_b` cùng chạy với mức ưu tiên 7, cùng tăng biến toàn cục `counter` 1000 lần (kỳ vọng: $2000$). Code chèn `k_yield()` giữa lúc đọc và ghi:
  * Khi `USE_MUTEX = 0`: Ghi đè lên nhau, `counter < 2000` (Sai dữ liệu, Y0 sáng báo lỗi).
  * Khi `USE_MUTEX = 1`: Vùng găng được bảo vệ, `counter = 2000` (Đúng chuẩn, LED3 sáng báo thành công).

---

### Demo 4: `lab2_demo4_msgq` - Mô hình Producer - Consumer với Message Queue
* **Mục tiêu:** Truyền cấu trúc dữ liệu lệnh an toàn giữa các luồng theo chuẩn FIFO.
* **Thiết kế:** Gói tin `struct led_cmd { uint8_t led; uint16_t ms; };` truyền qua `cmd_q` (dung lượng 4 phần tử). Producer đẩy lệnh, Consumer rút lệnh ra bật/tắt đúng đèn phần cứng. Tự động chuyển trạng thái Blocked khi hàng đợi đầy hoặc rỗng.

---

## 🛠️ 6. Danh mục 5 bài tập thực hành ứng dụng (`lab2_ex*`)

### Bài 1: `lab2_ex1_relay_binary_sem` - Chạy tiếp sức ba đèn bằng Binary Semaphore
* **Mục tiêu:** Dùng 3 Binary Semaphore để ép thứ tự thực thi tuần tự tuyệt đối giữa 3 luồng cùng mức ưu tiên (Priority 5).
* **Quy tắc:**
  - Ba luồng: `thread_led3`, `thread_y0`, `thread_y1`.
  - Ba semaphore: `sem_led3` (khởi tạo 1), `sem_y0` (khởi tạo 0), `sem_y1` (khởi tạo 0).
  - Chu trình: $\text{LED3} \rightarrow \text{Y0} \rightarrow \text{Y1} \rightarrow \text{LED3} \dots$ Mỗi đèn sáng 500 ms rồi tắt và chuyển lượt. Không bao giờ có 2 đèn cùng sáng.

---

### Bài 2: `lab2_ex2_timeout_monitor` - Giám sát tín hiệu ngắt có xử lý Timeout
* **Mục tiêu:** Xử lý giá trị trả về của `k_sem_take()` có timeout để phát hiện sự cố mất tín hiệu.
* **Kịch bản:**
  - `k_timer` phát ngắt chu kỳ: **1s $\rightarrow$ 1s $\rightarrow$ 4s** rồi lặp lại.
  - `thread_monitor` chờ tối đa 2000 ms: `k_sem_take(&sig_sem, K_MSEC(2000))`.
  - Trả về `0` (bình thường): In `Nhan tin hieu`, nháy Y0 hai lần.
  - Trả về `-EAGAIN` (quá 2s không có tín hiệu): In `TIMEOUT - mat tin hieu!` và đảo trạng thái LED3 cảnh báo.

---

### Bài 3: `lab2_ex3_counting_sem_workers` - Chia sẻ 2 kênh Output cho 4 Worker
* **Mục tiêu:** Quản lý nhóm tài nguyên (Resource Pool) bằng Counting Semaphore kết hợp Mutex bảo vệ bảng trạng thái kênh.
* **Cấu hình:** 2 kênh output Y0/Y1; 4 worker với thời gian làm việc/nghỉ: W1 (1000/500ms), W2 (1500/700ms), W3 (2000/300ms), W4 (2500/900ms). `pool_sem` (giá trị 2, 2) giới hạn tối đa 2 worker đồng thời; `ch_mutex` bảo vệ mảng `ch_busy[2]`. LED3 sáng khi không còn kênh trống.

---

### Bài 4: `lab2_ex4_isr_message_queue` - Thu thập dữ liệu từ ISR qua Message Queue
* **Mục tiêu:** Truyền dữ liệu từ ngắt sang luồng qua Message Queue, dùng biến nguyên tử `atomic_t` để thống kê tỷ lệ mất mẫu.
* **Thiết kế:** Timer 200 ms tạo mẫu `{seq, t_ms}` gửi vào `sample_q` với `K_NO_WAIT`. `thread_consumer` xử lý mất 500 ms (Phần a: mất mẫu) hoặc 150 ms (Phần b: không mất mẫu). `thread_monitor` in thống kê mỗi 5s và bật LED3 khi có mẫu bị mất.

---

### Bài 5: `lab2_ex5_pedestrian_traffic_light` - Đèn giao thông xin đường (Bài tổng hợp)
* **Mục tiêu:** Tích hợp tổng thể Semaphore có timeout, reset semaphore và Message Queue ghi log tập trung.
* **Kiến trúc:** Đèn giao thông Đỏ (7s) $\rightarrow$ Xanh (5s) $\rightarrow$ Vàng (2s). Nút bấm giả lập bằng timer (10s, 14s, 8s, 17s). Đèn Xanh sáng tối thiểu 2s, sau đó `k_sem_reset(&ped_sem)` và chờ 3s còn lại: nếu có nút bấm thì rút ngắn pha Xanh chuyển ngay sang Vàng. Duy nhất `thread_logger` (Priority 10) được gọi `printf` qua `log_q`.

---

## 💻 7. Hướng dẫn biên dịch & Nạp chương trình

Thực hiện lệnh với công cụ `west` từ thư mục gốc của repository:

```powershell
# 1. Kích hoạt môi trường Python ảo
cd $HOME\zephyrproject
.venv\Scripts\Activate.ps1

# 2. Biên dịch một bài tập cụ thể (Ví dụ: Bài 5 Đèn giao thông xin đường)
west build -p auto -b stm32f4_disco applications\lab2\lab2_ex5_pedestrian_traffic_light

# 3. Nạp firmware xuống Kit BKIT ARM4
west flash

# 4. Mở Serial Monitor theo dõi log UART (115200 baud, 8-N-1)
```

---

## ⚠️ 8. Các lỗi kỹ thuật thường gặp & Cách khắc phục

1. **Lỗi `blocked by your organization's Device Guard policy` (Mã 4551):**
   * *Nguyên nhân:* Tính năng **Smart App Control (SAC)** của Windows 11 chặn các file binary chưa ký số thương mại trong bộ Zephyr SDK (`arm-zephyr-eabi-ar.exe`).
   * *Khắc phục:* Vào **Windows Security** $\rightarrow$ **App & browser control** $\rightarrow$ **Smart App Control settings** $\rightarrow$ Chuyển sang **OFF**.
2. **Lỗi Crash / Kernel Panic khi gọi IPC trong ISR:**
   * *Nguyên nhân:* Gọi `k_mutex_lock()` hoặc gọi `k_sem_take` / `k_msgq_put` với timeout khác `K_NO_WAIT`.
   * *Khắc phục:* Trong ISR chỉ dùng timeout `K_NO_WAIT` và không bao giờ dùng Mutex.
3. **Lỗi Deadlock (Khóa chết):**
   * *Nguyên nhân:* Hai luồng cùng chờ tài nguyên của nhau theo vòng tròn (Circular Wait), hoặc quên give semaphore khởi đầu ở Bài 1 (cả 3 semaphore đều khởi tạo bằng 0).
   * *Khắc phục:* Luôn kiểm tra giá trị khởi tạo ban đầu và tuân thủ thứ tự chiếm giữ tài nguyên nhất quán.
