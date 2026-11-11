/*
 * ==============================================================================
 * BKIT ARM4 - Lab 2 - Demo 3: Mutex và Race Condition (Tranh chấp tài nguyên)
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI HỌC:
 *    - Hiểu khái niệm "Critical Section" (Vùng găng / Đoạn tranh chấp).
 *    - Hiểu "Race Condition" (Điều kiện tranh chấp): Xảy ra khi 2 hay nhiều thread cùng
 *      truy xuất và chỉnh sửa một tài nguyên dùng chung mà không có cơ chế khóa.
 *    - Nắm vững cách sử dụng Mutex (Mutual Exclusion - Loại trừ tương hỗ) để bảo vệ
 *      dữ liệu dùng chung, đảm bảo tính toàn vẹn (Data Integrity).
 *
 * 2. HIỆN TƯỢNG VÀ THÍ NGHIỆM:
 *    - Có 2 Worker Thread (A và B), mỗi bên tăng biến đếm 'counter' 1000 lần.
 *    - Kỳ vọng: counter sau cùng phải bằng đúng 2000 (1000 + 1000).
 *    - Thao tác "counter++" trong phần cứng thực chất gồm 3 bước:
 *        1. Đọc (Read): Tải giá trị từ RAM vào thanh ghi CPU (tmp = counter).
 *        2. Sửa (Modify): Tăng giá trị thanh ghi lên 1 (tmp + 1).
 *        3. Ghi (Write): Lưu giá trị từ thanh ghi trở lại RAM (counter = tmp + 1).
 *    - Code cố tình chèn k_yield() giữa lúc Đọc và Ghi để ép đổi ngữ cảnh (Context Switch):
 *        + Nếu USE_MUTEX = 0: Thread B chen ngang khi Thread A chưa kịp ghi -> dữ liệu bị ghi đè,
 *          kết quả cuối cùng < 2000 (Sai nghiêm trọng!). Đèn Y0 sẽ sáng báo lỗi.
 *        + Nếu USE_MUTEX = 1: Mutex khóa lại vùng găng. Dù Thread A có yield thì Thread B cũng
 *          không vào được vì bị chặn bởi k_mutex_lock. Kết quả ra đúng 2000. Đèn LED3 sáng báo đúng.
 *
 * 3. CÁC HÀM VÀ MACRO ZEPHYR QUAN TRỌNG:
 *    - K_MUTEX_DEFINE(name): Khởi tạo Mutex tĩnh lúc biên dịch.
 *    - k_mutex_lock(mutex, timeout): Khóa mutex (nếu đã bị giữ, thread sẽ bị block).
 *    - k_mutex_unlock(mutex): Mở khóa mutex (cho phép thread khác đang chờ được vào).
 *    - k_yield(): Tự nguyện nhường lượt CPU cho các thread khác có cùng mức ưu tiên.
 * ==============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/*
 * THỬ NGHIỆM:
 * - Đặt bằng 1: Bật Mutex bảo vệ -> Kết quả đúng 2000 -> LED3 sáng.
 * - Đặt bằng 0: Tắt Mutex -> Gặp lỗi Race Condition -> counter < 2000 -> Y0 sáng.
 */
#define USE_MUTEX 1
#define LOOPS     1000

/* Cấu hình các chân LED báo kết quả */
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios); /* Sáng khi kết quả ĐÚNG */
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);   /* Sáng khi kết quả SAI */

/* Tài nguyên dùng chung (Shared Resource) giữa 2 Thread */
static unsigned int counter;

/* Khởi tạo Mutex để bảo vệ biến 'counter' */
K_MUTEX_DEFINE(counter_mutex);

/*
 * Semaphore đếm dùng để báo hiệu khi nào 2 worker hoàn thành:
 * - Giá trị ban đầu = 0
 * - Giá trị tối đa = 2 (tương ứng 2 worker)
 */
K_SEM_DEFINE(done_sem, 0, 2);

/*
 * HÀM THỰC THI CỦA WORKER THREAD:
 * - Được chia sẻ dùng chung bởi cả Worker A và Worker B.
 */
static void thread_worker(void *p1, void *p2, void *p3)
{
	const char *name = (const char *)p1;

	for (int i = 0; i < LOOPS; i++) {
#if USE_MUTEX
		/* BẮT ĐẦU VÙNG GĂNG: Chỉ 1 thread duy nhất được sở hữu khóa này tại một thời điểm */
		k_mutex_lock(&counter_mutex, K_FOREVER);
#endif

		/* --- Đoạn mã mô phỏng thao tác Read-Modify-Write --- */
		unsigned int tmp = counter;   /* Bước 1: Đọc giá trị hiện tại */
		k_yield();                    /* Cố tình nhường CPU để kiểm tra xem có bị chen ngang không */
		counter = tmp + 1;            /* Bước 2 & 3: Tăng 1 và ghi lại vào bộ nhớ */

#if USE_MUTEX
		/* KẾT THÚC VÙNG GĂNG: Mở khóa để thread khác có cơ hội vào */
		k_mutex_unlock(&counter_mutex);
#endif
	}

	printf("%s xong\n", name);

	/* Tăng done_sem lên 1 báo cho thread main biết worker này đã làm xong */
	k_sem_give(&done_sem);
}

/*
 * KHỞI TẠO 2 THREAD WORKER VỚI MỨC ƯU TIÊN BẰNG NHAU (Priority = 7)
 * Nhờ cùng độ ưu tiên, k_yield() sẽ chuyển đổi qua lại giữa A và B.
 */
K_THREAD_DEFINE(worker_a, 1024, thread_worker, (void *)"Worker A", NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(worker_b, 1024, thread_worker, (void *)"Worker B", NULL, NULL, 7, 0, 0);

int main(void)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	printf("Lab 2 - Demo 3: USE_MUTEX = %d\n", USE_MUTEX);

	/*
	 * CƠ CHẾ CHỜ THREAD HOÀN TẤT (Thread Join bằng Semaphore):
	 * - Main thread gọi k_sem_take lần 1: Đợi worker đầu tiên làm xong (give lần 1).
	 * - Main thread gọi k_sem_take lần 2: Đợi worker thứ hai làm xong (give lần 2).
	 */
	k_sem_take(&done_sem, K_FOREVER);
	k_sem_take(&done_sem, K_FOREVER);

	/* Kiểm tra và in kết quả */
	printf("counter = %u (mong doi %d) -> %s\n", counter, 2 * LOOPS,
	       counter == 2 * LOOPS ? "DUNG" : "SAI (race condition!)");

	/* Đèn LED3 sáng = kết quả ĐÚNG; Đèn Y0 sáng = kết quả SAI */
	gpio_pin_set_dt(counter == 2 * LOOPS ? &led3 : &y0, 1);

	return 0;
}
