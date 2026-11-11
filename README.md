# 🔄 LAB 2: ĐỒNG BỘ TIẾN TRÌNH VÀ TRUYỀN THÔNG LIÊN TIẾN TRÌNH (IPC) TRONG ZEPHYR RTOS
### Lý thuyết chuyên sâu về Semaphore, Mutex Thừa kế ưu tiên, Message Queue, Deferred ISR & Bài tập ứng dụng

> **Trường Đại học Bách Khoa - ĐHQG-HCM**  
> **Khoa Khoa học và Kỹ thuật Máy tính**  
> **Môn học:** Hệ thống nhúng (Embedded Systems)  
> **Sinh viên thực hiện:** Võ Phạm (MSSV: 2313946)  
> **Nhánh Git:** `lab2_synchronization` | **Nền tảng:** Kit BKIT ARM4 (STM32F407ZGT6, ARM Cortex-M4 @ 168 MHz)  
> **Hệ điều hành:** Zephyr RTOS v4.4.0 (Công cụ quản trị: `west`)

---

## 📖 Mục lục
- [1. Mục tiêu thí nghiệm](#1-mục-tiêu-thí-nghiệm)
- [2. Kiến thức nền tảng & Lý thuyết chuyên sâu về IPC](#2-kiến-thức-nền-tảng--lý-thuyết-chuyên-sâu-về-ipc)
  - [2.1. Bản chất vì sao cần đồng bộ hóa trong RTOS?](#21-bản-chất-vì-sao-cần-đồng-bộ-hóa-trong-rtos)
  - [2.2. Semaphore: Binary Semaphore và Counting Semaphore](#22-semaphore-binary-semaphore-và-counting-semaphore)
  - [2.3. Mutex và Cơ chế Thừa kế độ ưu tiên (Priority Inheritance)](#23-mutex-và-cơ-chế-thừa-kế-độ-ưu-tiên-priority-inheritance)
  - [2.4. Message Queue: Cơ chế hàng đợi vòng FIFO](#24-message-queue-cơ-chế-hàng-đợi-vòng-fifo)
  - [2.5. Hướng dẫn lựa chọn công cụ IPC phù hợp](#25-hướng-dẫn-lựa-chọn-công-cụ-ipc-phù-hợp)
  - [2.6. Cơ chế Timeout và Mã lỗi trả về của Kernel](#26-cơ-chế-timeout-và-mã-lỗi-trả-về-của-kernel)
  - [2.7. Nguyên tắc vàng trong ngữ cảnh ngắt phần cứng (ISR Context)](#27-nguyên-tắc-vàng-trong-ngữ-cảnh-ngắt-phần-cứng-isr-context)
  - [2.8. Bảng đối chiếu cú pháp: Zephyr RTOS vs FreeRTOS vs CMSIS-RTOS2](#28-bảng-đối-chiếu-cú-pháp-zephyr-rtos-vs-freertos-vs-cmsis-rtos2)
- [3. Cấu trúc thư mục dự án](#3-cấu-trúc-thư-mục-dự-án)
- [4. Danh mục 4 bài Demo nền tảng (demos/)](#4-danh-mục-4-bài-demo-nền-tảng-demos)
  - [Demo 1: lab2_demo1_binary_sem - Xử lý ngắt hoãn lại (Deferred ISR)](#demo-1-lab2_demo1_binary_sem---xử-lý-ngắt-hoãn-lại-deferred-isr)
  - [Demo 2: lab2_demo2_counting_sem - Đệm sự kiện dồn dập (Burst Handling)](#demo-2-lab2_demo2_counting_sem---đệm-sự-kiện-dồn-dập-burst-handling)
  - [Demo 3: lab2_demo3_mutex - Khắc phục Race Condition bằng Mutex](#demo-3-lab2_demo3_mutex---khắc-phục-race-condition-bằng-mutex)
  - [Demo 4: lab2_demo4_msgq - Mô hình Producer - Consumer với Message Queue](#demo-4-lab2_demo4_msgq---mô-hình-producer---consumer-với-message-queue)
- [5. Danh mục 5 bài tập thực hành ứng dụng (lab2_ex*)](#5-danh-mục-5-bài-tập-thực-hành-ứng-dụng-lab2_ex)
  - [Bài 1: lab2_ex1_relay_binary_sem - Chạy tiếp sức ba đèn bằng Binary Semaphore](#bài-1-lab2_ex1_relay_binary_sem---chạy-tiếp-sức-ba-đèn-bằng-binary-semaphore)
  - [Bài 2: lab2_ex2_timeout_monitor - Giám sát tín hiệu ngắt có xử lý Timeout](#bài-2-lab2_ex2_timeout_monitor---giám-sát-tín-hiệu-ngắt-có-xử-lý-timeout)
  - [Bài 3: lab2_ex3_counting_sem_workers - Chia sẻ 2 kênh Output cho 4 Worker](#bài-3-lab2_ex3_counting_sem_workers---chia-sẻ-2-kênh-output-cho-4-worker)
  - [Bài 4: lab2_ex4_isr_message_queue - Thu thập dữ liệu từ ISR qua Message Queue](#bài-4-lab2_ex4_isr_message_queue---thu-thập-dữ-liệu-từ-isr-qua-message-queue)
  - [Bài 5: lab2_ex5_pedestrian_traffic_light - Đèn giao thông xin đường (Bài tổng hợp)](#bài-5-lab2_ex5_pedestrian_traffic_light---đèn-giao-thông-xin-đường-bài-tổng-hợp)
- [6. Hướng dẫn biên dịch & Nạp chương trình](#6-hướng-dẫn-biên-dịch--nạp-chương-trình)
- [7. Tổng kết các lỗi thường gặp & Cách khắc phục](#7-tổng-kết-các-lỗi-thường-gặp--cách-khắc-phục)

---

## 🎯 1. Mục tiêu thí nghiệm

1. **Hiểu sâu sắc bản chất đồng bộ và truyền thông:** Nắm vững tại sao các hệ thống nhúng đa tác vụ bắt buộc phải sử dụng các cơ chế đồng bộ cấp hệ điều hành (OS-level primitives) thay vì các biến cờ đơn giản.
2. **Làm chủ các công cụ IPC cốt lõi của Zephyr RTOS:**
   - **Binary Semaphore (`k_sem`):** Đồng bộ báo hiệu sự kiện 1-1, kỹ thuật Deferred Interrupt Processing.
   - **Counting Semaphore:** Quản lý nhóm tài nguyên giống nhau (Resource Pool) và đếm các sự kiện dồn dập.
   - **Mutex (`k_mutex`):** Bảo vệ vùng găng (Critical Section) chống tranh chấp dữ liệu (Race Condition) và triệt tiêu nghịch đảo ưu tiên (Priority Inversion).
   - **Message Queue (`k_msgq`):** Hàng đợi vòng truyền nhận dữ liệu có cấu trúc an toàn theo nguyên tắc FIFO.
3. **Thực thi chuẩn mực an toàn trong ngắt (ISR Safety):** Tuyệt đối không gọi hàm blocking trong ISR, xử lý timeout chính xác với `K_NO_WAIT`.

---

## 📚 2. Kiến thức nền tảng & Lý thuyết chuyên sâu về IPC

### 2.1. Bản chất vì sao cần đồng bộ hóa trong RTOS?
Trong hệ điều hành thời gian thực, các luồng (thread) chạy đan xen thông qua cơ chế phân chia thời gian (Time-slicing) hoặc chiếm quyền theo độ ưu tiên (Preemption). Hai vấn đề kinh điển nảy sinh:
1. **Tranh chấp dữ liệu (Race Condition):** Khi hai hoặc nhiều luồng cùng đọc và sửa đổi một biến hoặc tài nguyên phần cứng dùng chung. Thao tác tưởng chừng là 1 lệnh trong C như `counter++` thực chất gồm 3 bước máy:
   $$\text{Đọc (Read RAM } \to \text{ Register)} \longrightarrow \text{Biến đổi (Modify Register)} \longrightarrow \text{Ghi (Write Register } \to \text{ RAM)}$$
   Nếu luồng bị đổi ngữ cảnh (Context Switch) ở giữa 3 bước này, dữ liệu sẽ bị ghi đè sai lệch hoàn toàn.
2. **Phối hợp thứ tự thực thi (Signaling):** Luồng B cần đợi một sự kiện phần cứng do ngắt (ISR) hoặc do Luồng A tạo ra rồi mới được phép chạy. Nếu dùng vòng lặp chờ cờ `while (!flag) {}`, CPU sẽ bị chiếm dụng 100% vô ích (Busy-waiting). Cơ chế đồng bộ giúp đưa luồng vào trạng thái **BLOCKED/WAITING**, giải phóng hoàn toàn chu kỳ CPU cho các tác vụ khác.

---

### 2.2. Semaphore: Binary Semaphore và Counting Semaphore

Trong Zephyr RTOS, một Semaphore là một biến đếm nguyên không âm kèm theo một hàng đợi các luồng đang chờ:
- **`initial_count`:** Giá trị khởi tạo ban đầu ($\ge 0$).
- **`limit`:** Cận trên tối đa mà biến đếm có thể đạt tới.

```c
K_SEM_DEFINE(my_sem, initial_count, limit);
```

#### Hai hàm nguyên tử cốt lõi:
1. **`k_sem_give(&my_sem)`:**
   - Nếu có ít nhất một luồng đang chờ trong hàng đợi: Đánh thức luồng có độ ưu tiên cao nhất sang trạng thái READY. Biến đếm giữ nguyên.
   - Nếu không có luồng nào chờ: Tăng biến đếm lên 1 (nếu chưa chạm ngưỡng `limit`). Nếu đã chạm `limit`, lệnh give bị bỏ qua không tăng thêm.
   - *An toàn tuyệt đối khi gọi từ bên trong ISR.*
2. **`k_sem_take(&my_sem, timeout)`:**
   - Nếu biến đếm $> 0$: Giảm biến đếm đi 1 và tiếp tục chạy ngay lập tức mà không chờ (trả về `0`).
   - Nếu biến đếm $= 0$: Đưa luồng gọi vào hàng đợi BLOCKED cho đến khi có ai give hoặc hết thời gian `timeout` (trả về `-EAGAIN`).

#### Phân biệt Binary Semaphore và Counting Semaphore:
| Đặc tính | Binary Semaphore (`limit = 1`) | Counting Semaphore (`limit = N > 1`) |
| :--- | :--- | :--- |
| **Khoảng giá trị** | Chỉ nhận giá trị `0` hoặc `1` | Nhận giá trị từ `0` đến `N` |
| **Mục đích chính** | **Báo hiệu sự kiện 1-1 (Signaling)**: Ngắt báo cho Thread, hoặc Thread chuyển lượt cho Thread khác. | **Quản lý tài nguyên (Resource Pool)**: Có $N$ kênh khả dụng, cấp phát cho nhiều worker; hoặc **Đệm số lượng sự kiện** dồn dập. |
| **Bộ đệm tín hiệu** | Không có bộ đệm (nếu give 2 lần liên tiếp trước khi take, lần 2 bị mất). | Có bộ đệm lưu được tối đa $N$ sự kiện chưa kịp xử lý. |

---

### 2.3. Mutex và Cơ chế Thừa kế độ ưu tiên (Priority Inheritance)

**Mutex (Mutual Exclusion)** được thiết kế chuyên biệt để bảo vệ **Vùng găng (Critical Section)** của tài nguyên dùng chung.

```c
K_MUTEX_DEFINE(my_mutex);
k_mutex_lock(&my_mutex, K_FOREVER);
/* --- VÙNG GĂNG (Chỉ duy nhất 1 thread được chạy đoạn này tại một thời điểm) --- */
k_mutex_unlock(&my_mutex);
```

#### So sánh then chốt: Mutex khác gì Binary Semaphore?
Nhiều lập trình viên nhầm lẫn có thể dùng Binary Semaphore thay thế cho Mutex. Đây là sai lầm nguy hiểm trong hệ thống nhúng vì 2 khác biệt bản chất:
1. **Quyền sở hữu (Ownership):** Mutex có khái niệm chủ sở hữu. Chỉ có thread nào đã gọi `k_mutex_lock()` mới CÓ QUYỀN gọi `k_mutex_unlock()`. Trình phục vụ ngắt (ISR) tuyệt đối không có quyền lock/unlock Mutex. Ngược lại, Semaphore không có chủ sở hữu: Thread A take nhưng Thread B hoặc ISR có thể give!
2. **Cơ chế Thừa kế độ ưu tiên (Priority Inheritance):**
   * *Hiện tượng Đảo ngược ưu tiên (Priority Inversion):* Giả sử Luồng Thấp (L) giữ khóa; Luồng Cao (H) vào tranh chấp và bị chặn. Luồng Trung bình (M) không cần khóa xuất hiện và chiếm quyền chạy của L (vì ưu tiên $M > L$). Hậu quả: Luồng H (rất quan trọng) bị gián tiếp chặn bởi Luồng M (kém quan trọng hơn)!
   * *Giải pháp của Mutex:* Khi Luồng H bị chặn tại Mutex do Luồng L đang giữ, Kernel Zephyr **tự động nâng tạm thời độ ưu tiên của L lên bằng H**. Nhờ đó L chạy nhanh chóng để hoàn tất vùng găng, unlock Mutex rồi lập tức trả lại quyền chạy cho H. Khi xong, ưu tiên của L hạ về mức ban đầu. Binary Semaphore **hoàn toàn không có** cơ chế bảo vệ này!

---

### 2.4. Message Queue: Cơ chế hàng đợi vòng FIFO

Khi hai luồng cần trao đổi không chỉ là tín hiệu "có/không" mà là **cả một gói dữ liệu hoàn chỉnh** (cấu trúc `struct`), **Message Queue** là lựa chọn chuẩn mực:

```c
K_MSGQ_DEFINE(my_msgq, msg_size, max_msgs, align);
```
- **Nguyên lý hoạt động:** Dữ liệu được sao chép theo giá trị (Pass-by-value / Memory Copy) vào bộ đệm vòng (Ring Buffer) nội bộ của Kernel.
- **`k_msgq_put(&my_msgq, &data, timeout)`:** Đẩy thông điệp vào cuối hàng đợi. Nếu hàng đợi đã đầy `max_msgs`, luồng gọi sẽ bị block chờ cho đến khi có chỗ trống (trong ISR bắt buộc dùng `K_NO_WAIT`).
- **`k_msgq_get(&my_msgq, &data, timeout)`:** Lấy thông điệp ở đầu hàng đợi (First-In, First-Out). Nếu hàng đợi rỗng, luồng gọi tự động chuyển sang trạng thái ngủ chờ dữ liệu mới.

---

### 2.5. Hướng dẫn lựa chọn công cụ IPC phù hợp

```
                    BẠN CẦN GIẢI QUYẾT BÀI TOÁN GÌ?
                                   │
         ┌─────────────────────────┴────────────────────────┐
   Cần truyền DỮ LIỆU?                                Cần ĐỒNG BỘ / KHÓA?
         │                                                  │
   [MESSAGE QUEUE]                               ┌──────────┴──────────┐
(Truyền struct, byte array,               Bảo vệ VÙNG GĂNG?       Báo hiệu SỰ KIỆN?
 ring buffer an toàn FIFO)                 (Shared Memory)        (Signaling)
                                                 │                      │
                                              [MUTEX]        ┌──────────┴──────────┐
                                       (Có Priority Inh.,  Chỉ báo 1 sự kiện?   Quản lý Pool /
                                        bắt buộc unlock     (ISR -> Thread)      Đếm nhiều sự kiện?
                                        bởi chính chủ)           │                      │
                                                         [BINARY SEMAPHORE]   [COUNTING SEMAPHORE]
```

---

### 2.6. Cơ chế Timeout và Mã lỗi trả về của Kernel

Các hàm `k_sem_take`, `k_mutex_lock`, `k_msgq_put`, `k_msgq_get` đều nhận tham số `k_timeout_t`:
- **`K_FOREVER`:** Chờ vô thời hạn cho tới khi tài nguyên khả dụng.
- **`K_NO_WAIT`:** Thăm dò tức thì. Nếu tài nguyên chưa sẵn sàng, trả về mã lỗi ngay lập tức, không đưa luồng vào hàng đợi chờ.
- **`K_MSEC(ms)` / `K_SECONDS(s)`:** Chờ tối đa khoảng thời gian chỉ định.

#### Giá trị trả về chuẩn:
* **`0`:** Thao tác thành công hoàn toàn trong thời gian cho phép.
* **`-EAGAIN`:** Hết thời gian chờ (Timeout) mà tài nguyên vẫn chưa sẵn sàng.
* **`-ENOMSG`:** Hàng đợi tin nhắn rỗng khi gọi với `K_NO_WAIT`.
* **`-EBUSY`:** Tài nguyên đang bận khi thăm dò không chờ.

---

### 2.7. Nguyên tắc vàng trong ngữ cảnh ngắt phần cứng (ISR Context)

Trình phục vụ ngắt phần cứng (ISR) chạy với đặc quyền cao nhất và **không có ngữ cảnh luồng riêng (No Thread Context)**. Do đó:
1. ❌ **CẤM tuyệt đối** gọi các hàm gây trễ hoặc ngủ: `k_msleep()`, `k_yield()`, `k_busy_wait()`.
2. ❌ **CẤM** gọi `k_mutex_lock()` hoặc `k_mutex_unlock()` (Mutex yêu cầu thread context).
3. ❌ **CẤM** gọi `k_sem_take()` với timeout khác `K_NO_WAIT`.
4. ❌ **KHÔNG NÊN** gọi `printf()` hay in UART dài dòng trong ISR vì làm trễ các ngắt khác.
5.  **BẮT BUỘC** dùng timeout `K_NO_WAIT` khi gọi `k_msgq_put()` hoặc `k_sem_take()`.
6.  **Kỹ thuật chuẩn:** *Deferred Interrupt Processing* — ISR chỉ làm nhiệm vụ ghi nhận sự kiện (give semaphore hoặc push message queue với `K_NO_WAIT`) rồi thoát ngay trong vài micro-giây.

---

### 2.8. Bảng đối chiếu cú pháp: Zephyr RTOS vs FreeRTOS vs CMSIS-RTOS2

| Tính năng | Zephyr RTOS v4.4.0 | FreeRTOS | CMSIS-RTOS2 (STM32 HAL) |
| :--- | :--- | :--- | :--- |
| **Khai báo Semaphore** | `K_SEM_DEFINE(sem, init, max);` | `xSemaphoreCreateCounting(max, init);` | `osSemaphoreNew(max, init, NULL);` |
| **Nhận Semaphore** | `k_sem_take(&sem, timeout);` | `xSemaphoreTake(sem, ticks);` | `osSemaphoreAcquire(sem, timeout);` |
| **Nhả Semaphore (Thread)**| `k_sem_give(&sem);` | `xSemaphoreGive(sem);` | `osSemaphoreRelease(sem);` |
| **Nhả Semaphore (ISR)** | `k_sem_give(&sem);` | `xSemaphoreGiveFromISR(sem, &px);` | `osSemaphoreRelease(sem);` |
| **Khai báo Mutex** | `K_MUTEX_DEFINE(mutex);` | `xSemaphoreCreateMutex();` | `osMutexNew(NULL);` |
| **Khóa Mutex** | `k_mutex_lock(&mutex, timeout);` | `xSemaphoreTake(mutex, ticks);` | `osMutexAcquire(mutex, timeout);` |
| **Mở khóa Mutex** | `k_mutex_unlock(&mutex);` | `xSemaphoreGive(mutex);` | `osMutexRelease(mutex);` |
| **Khai báo Message Queue**| `K_MSGQ_DEFINE(q, size, num, 4);`| `xQueueCreate(num, size);` | `osMessageQueueNew(num, size, NULL);` |
| **Gửi Message** | `k_msgq_put(&q, &data, timeout);`| `xQueueSend(q, &data, ticks);` | `osMessageQueuePut(q, &data, 0, to);` |
| **Nhận Message** | `k_msgq_get(&q, &data, timeout);`| `xQueueReceive(q, &data, ticks);` | `osMessageQueueGet(q, &data, NULL, to);`|

---

## 📁 3. Cấu trúc thư mục dự án

```text
EMBEDDED-SYSTEMS_Kit-BKIT-ARM4/ (nhánh lab2_synchronization)
├── demos/                              # Thư mục chứa 4 bài Demo khảo sát nguyên lý
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
└── README.md                           # Tài liệu kỹ thuật chi tiết Lab 2
```

---

## 🔬 4. Danh mục 4 bài Demo nền tảng (`demos/`)

### Demo 1: `lab2_demo1_binary_sem` - Xử lý ngắt hoãn lại (Deferred ISR)
* **Mục tiêu:** Ứng dụng Binary Semaphore để chuyển tác vụ nặng từ ngắt sang luồng xử lý riêng biệt.
* **Thiết kế:**
  - Timer hệ điều hành (`k_timer`) tạo ngắt chu kỳ mỗi 2 giây. Trong ISR callback, chỉ thực hiện lệnh duy nhất: `k_sem_give(&evt_sem)`.
  - Luồng `thread_handler` bị block tại `k_sem_take(&evt_sem, K_FOREVER)`. Khi nhận tín hiệu, luồng thức giấc và thực hiện nháy LED3 3 lần (chu kỳ 100ms sáng / 100ms tắt, tổng thời gian 600 ms).
* **Kết quả:** Ngắt kết thúc chỉ trong vài micro-giây, CPU không bị chiếm giữ vô lý trong ISR.

---

### Demo 2: `lab2_demo2_counting_sem` - Đệm sự kiện dồn dập (Burst Handling)
* **Mục tiêu:** Quan sát khả năng tích lũy sự kiện của Counting Semaphore khi nguồn phát nhanh hơn bộ phận xử lý.
* **Kịch bản Burst:**
  - Timer phát nhanh một đợt 15 sự kiện cách nhau 300 ms (tổng thời gian phát 4.5 giây).
  - Consumer Thread xử lý mỗi sự kiện mất 1000 ms (700 ms sáng Y0, 300 ms tắt Y0).
  - Semaphore được định nghĩa tối đa `EVT_LIMIT = 5`: `K_SEM_DEFINE(evt_count, 0, 5)`.
* **Kết luận thực nghiệm:** Do bộ đệm chỉ chứa được 5 sự kiện, khi nguồn phát bắn quá nhanh vượt mức chứa, 5 sự kiện được giữ lại xử lý lần lượt sau đó, và các sự kiện vượt quá bị mất mát (`lost`).

---

### Demo 3: `lab2_demo3_mutex` - Khắc phục Race Condition bằng Mutex
* **Mục tiêu:** Tái hiện hiện tượng tranh chấp dữ liệu (Race Condition) và chứng minh hiệu quả bảo vệ của Mutex.
* **Thực nghiệm:**
  - Hai luồng `worker_a` và `worker_b` cùng chạy với mức ưu tiên 7, cùng tăng biến toàn cục `counter` 1000 lần (kỳ vọng: $2000$).
  - Giữa bước đọc và ghi, code cố tình gọi `k_yield()` để kích hoạt đổi ngữ cảnh.
  - Khi `USE_MUTEX = 0`: Hai luồng ghi đè lên nhau, `counter` sau cùng chỉ đạt $< 2000$ (Sai dữ liệu nghiêm trọng! Đèn Y0 sáng báo lỗi).
  - Khi `USE_MUTEX = 1`: Vùng găng được khóa bởi `k_mutex_lock()`. Kết quả ra đúng $2000$ (Đèn LED3 sáng báo thành công).

---

### Demo 4: `lab2_demo4_msgq` - Mô hình Producer - Consumer với Message Queue
* **Mục tiêu:** Truyền cấu trúc dữ liệu lệnh an toàn giữa các luồng theo chuẩn FIFO.
* **Thiết kế:**
  - Định nghĩa gói tin: `struct led_cmd { uint8_t led; uint16_t ms; };`
  - Hàng đợi: `K_MSGQ_DEFINE(cmd_q, sizeof(struct led_cmd), 4, 4);`
  - `thread_producer` đẩy liên tiếp các mẫu hiệu ứng ánh sáng vào hàng đợi. Khi hàng đầy (4/4), luồng tự động ngủ chờ.
  - `thread_consumer` rút từng lệnh ra qua `k_msgq_get()` và điều khiển bật/tắt đúng đèn phần cứng theo thời gian chỉ định.

---

## 🛠️ 5. Danh mục 5 bài tập thực hành ứng dụng (`lab2_ex*`)

### Bài 1: `lab2_ex1_relay_binary_sem` - Chạy tiếp sức ba đèn bằng Binary Semaphore
* **Mục tiêu:** Dùng 3 Binary Semaphore để ép thứ tự thực thi tuần tự tuyệt đối giữa 3 luồng có cùng mức ưu tiên (Priority 5).
* **Quy tắc:**
  - Ba luồng: `thread_led3`, `thread_y0`, `thread_y1`.
  - Ba semaphore: `sem_led3` (khởi tạo 1), `sem_y0` (khởi tạo 0), `sem_y1` (khởi tạo 0).
  - Thứ tự tuần tự: $\text{LED3} \rightarrow \text{Y0} \rightarrow \text{Y1} \rightarrow \text{LED3} \rightarrow \dots$
  - Mỗi đèn sáng 500 ms rồi tắt và give semaphore cho đèn kế tiếp. Đảm bảo tại mọi thời điểm chỉ có duy nhất 1 đèn sáng. Không dùng cờ biến toàn cục hay `busy-wait`.
* **Mẫu Log chuẩn:**
  ```text
  [  1000 ms] LED3
  [  1500 ms] Y0
  [  2000 ms] Y1
  [  2500 ms] LED3
  ```

---

### Bài 2: `lab2_ex2_timeout_monitor` - Giám sát tín hiệu ngắt có xử lý Timeout
* **Mục tiêu:** Xử lý giá trị trả về của `k_sem_take()` có timeout để phát hiện sự cố mất tín hiệu.
* **Kịch bản:**
  - `k_timer` one-shot phát tín hiệu ngắt theo chu kỳ: **1s $\rightarrow$ 1s $\rightarrow$ 4s** rồi lặp lại. Trong ISR chỉ give semaphore và khởi động lại timer.
  - `thread_monitor` chờ tối đa 2000 ms bằng lệnh: `k_sem_take(&sig_sem, K_MSEC(2000))`.
  - Khi trả về `0` (bình thường): In `Nhan tin hieu`, nháy Y0 hai lần (100ms bật / 100ms tắt).
  - Khi trả về `-EAGAIN` (quá 2000ms không có tín hiệu): In `TIMEOUT - mat tin hieu!` và đảo trạng thái LED3 cảnh báo.

---

### Bài 3: `lab2_ex3_counting_sem_workers` - Chia sẻ 2 kênh Output cho 4 Worker
* **Mục tiêu:** Quản lý nhóm tài nguyên (Resource Pool) bằng Counting Semaphore kết hợp Mutex bảo vệ bảng trạng thái kênh.
* **Cấu hình:**
  - 2 kênh output: Y0 (kênh 0) và Y1 (kênh 1).
  - 4 worker với cấu hình thời gian riêng biệt:
    - `W1`: làm việc 1000 ms, nghỉ 500 ms.
    - `W2`: làm việc 1500 ms, nghỉ 700 ms.
    - `W3`: làm việc 2000 ms, nghỉ 300 ms.
    - `W4`: làm việc 2500 ms, nghỉ 900 ms.
  - `pool_sem` (giá trị 2, 2) đảm bảo không bao giờ có quá 2 worker dùng kênh cùng lúc.
  - `ch_mutex` bảo vệ mảng `bool ch_busy[2]`. LED3 sáng khi cả 2 kênh đều bận (`k_sem_count_get(&pool_sem) == 0`).

---

### Bài 4: `lab2_ex4_isr_message_queue` - Thu thập dữ liệu từ ISR qua Message Queue
* **Mục tiêu:** Truyền dữ liệu từ ngắt sang luồng qua Message Queue, dùng biến nguyên tử `atomic_t` để thống kê tỷ lệ mất mẫu.
* **Thiết kế:**
  - Timer chu kỳ 200 ms: trong ISR tạo mẫu `{seq, t_ms}` và gửi vào `sample_q` (sức chứa 8 mẫu) bằng `k_msgq_put(..., K_NO_WAIT)`. Nếu thành công tăng `n_sent`, nếu đầy tăng `n_drop`.
  - `thread_consumer`: Lấy mẫu ra xử lý mất 500 ms (Phần a: chậm hơn tốc độ phát $\to$ bị mất mẫu); hoặc 150 ms (Phần b: nhanh hơn tốc độ phát $\to$ không mất mẫu).
  - `thread_monitor`: Định kỳ mỗi 5s in thống kê `GUI / NHAN / MAT / TRONG HANG DOI` và bật LED3 khi có mẫu bị mất.

---

### Bài 5: `lab2_ex5_pedestrian_traffic_light` - Đèn giao thông xin đường (Bài tổng hợp)
* **Mục tiêu:** Tích hợp tổng thể Semaphore có timeout, reset semaphore và Message Queue ghi log tập trung.
* **Kiến trúc:**
  - `thread_traffic_light`: Chu kỳ ĐỎ (7s) $\rightarrow$ XANH (5s) $\rightarrow$ VÀNG (2s).
  - Người đi bộ bấm nút giả lập bằng timer: lần đầu ở 10s, sau đó cách nhau 14s, 8s, 17s.
  - **Quy tắc ưu tiên:** Đèn XANH sáng tối thiểu 2s. Sau 2s, gọi `k_sem_reset(&ped_sem)` xóa yêu cầu cũ, rồi chờ yêu cầu mới trong 3s còn lại qua `k_sem_take(&ped_sem, K_MSEC(3000))`. Nếu có yêu cầu, rút ngắn pha Xanh chuyển ngay sang Vàng.
  - **Ghi log tập trung:** Duy nhất `thread_logger` (Priority 10 - thấp nhất) được gọi `printf`. Mọi sự kiện đổi pha, bấm nút, rút ngắn pha đều đóng gói vào `struct evt` và gửi qua `log_q`.
  - `thread_led_blinky`: Nháy LED3 chu kỳ 1s/1s báo nhịp tim hệ thống.

---

## 💻 6. Hướng dẫn biên dịch & Nạp chương trình

Thực hiện lệnh với công cụ `west` từ thư mục gốc của repository:

```powershell
# 1. Kích hoạt môi trường Python ảo
cd $HOME\zephyrproject
.venv\Scripts\Activate.ps1

# 2. Biên dịch một bài tập cụ thể (Ví dụ: Bài 5 Đèn giao thông xin đường)
west build -p auto -b stm32f4_disco applications\lab2\lab2_ex5_pedestrian_traffic_light

# 3. Nạp firmware xuống Kit BKIT ARM4
west flash

# 4. Mở Serial Monitor theo dõi log (115200 baud, 8-N-1)
```

---

## ⚠️ 7. Tổng kết các lỗi thường gặp & Cách khắc phục

1. **Lỗi `was blocked by your organization's Device Guard policy` (Mã 4551):**
   * *Nguyên nhân:* Tính năng **Smart App Control (SAC)** của Windows 11 chặn các file binary chưa ký số thương mại trong bộ Zephyr SDK (`arm-zephyr-eabi-ar.exe`).
   * *Khắc phục:* Vào Windows Security $\rightarrow$ App & browser control $\rightarrow$ Smart App Control settings $\rightarrow$ Chuyển sang **OFF**.
2. **Lỗi Crash / Panic khi gọi IPC trong ISR:**
   * *Nguyên nhân:* Gọi `k_mutex_lock()` hoặc gọi `k_sem_take` / `k_msgq_put` với timeout khác `K_NO_WAIT`.
   * *Khắc phục:* Trong ISR chỉ dùng timeout `K_NO_WAIT` và không dùng Mutex.
3. **Lỗi Deadlock (Khóa chết):**
   * *Nguyên nhân:* Hai luồng cùng chờ tài nguyên của nhau theo vòng tròn (Circular Wait), hoặc quên give semaphore khởi đầu ở Bài 1.
   * *Khắc phục:* Luôn kiểm tra giá trị khởi tạo ban đầu của Semaphore và tuân thủ thứ tự khóa tài nguyên nhất quán.
