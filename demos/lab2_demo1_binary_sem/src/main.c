/*
 * ==============================================================================
 * BKIT ARM4 - Lab 2 - Demo 1: Binary Semaphore (Đồng bộ: Ngắt -> Thread)
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI HỌC:
 *    - Hiểu cơ chế hoạt động của Binary Semaphore (semaphore nhị phân: giá trị 0 hoặc 1).
 *    - Nắm vững kỹ thuật "Deferred Interrupt Processing" (Xử lý ngắt hoãn lại):
 *        + Trình phục vụ ngắt (ISR) phải chạy CỰC KỲ NHANH, không được phép gọi các hàm
 *          gây trễ (blocking) như k_msleep().
 *        + Do đó, ISR chỉ làm nhiệm vụ báo hiệu ("give" semaphore) rồi thoát ngay.
 *        + Một thread riêng biệt sẽ chờ ("take" semaphore) và thực hiện các công việc
 *          nặng/tốn thời gian (ở đây là nháy LED 3 lần kèm trễ 600ms).
 *
 * 2. CÁC HÀM VÀ MACRO ZEPHYR QUAN TRỌNG:
 *    - K_SEM_DEFINE(name, initial_count, count_limit): Khởi tạo semaphore lúc biên dịch.
 *    - k_sem_give(sem): Tăng giá trị semaphore (đánh thức thread đang đợi). An toàn khi gọi từ ISR.
 *    - k_sem_take(sem, timeout): Chờ và giảm giá trị semaphore. Đưa thread vào trạng thái chờ (Blocked),
 *                                không tiêu tốn chu kỳ CPU.
 *    - K_TIMER_DEFINE(name, expiry_fn, stop_fn): Tạo bộ định thời của hệ điều hành.
 *    - k_timer_start(timer, duration, period): Kích hoạt timer chạy một lần hoặc chu kỳ.
 *    - K_THREAD_DEFINE(...): Khai báo và tự động khởi chạy một thread trong Zephyr.
 * ==============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/*
 * Cấu hình chân GPIO cho LED từ Device Tree (alias: led0, thường là LED3 trên board STM32F4-Discovery)
 */
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/*
 * KHỞI TẠO BINARY SEMAPHORE:
 * - Tên: evt_sem
 * - Giá trị ban đầu (initial_count): 0 (nghĩa là ban đầu chưa có sự kiện nào, thread sẽ bị block nếu take)
 * - Giá trị tối đa (count_limit): 1 (đặc trưng của Binary Semaphore: chỉ chứa tối đa 1 tín hiệu)
 */
K_SEM_DEFINE(evt_sem, 0, 1);

/*
 * HÀM XỬ LÝ NGẮT (ISR) CỦA TIMER:
 * - Chạy trong ISR context (ngữ cảnh ngắt phần cứng):
 *   + NGUYÊN TẮC VÀNG: Không được dùng vòng lặp dài, không được gọi k_msleep() hay k_yield().
 *   + Công việc duy nhất: Gửi tín hiệu đánh thức thread bằng k_sem_give().
 */
static void timer_isr(struct k_timer *timer)
{
	/* Tăng evt_sem từ 0 lên 1. Nếu có thread đang chờ ở k_sem_take, thread đó sẽ được chuyển sang Ready */
	k_sem_give(&evt_sem);
}

/*
 * KHỞI TẠO TIMER:
 * - Tên: evt_timer
 * - expiry_fn: timer_isr (hàm callback được gọi mỗi khi timer đếm hết giờ)
 * - stop_fn: NULL (không cần callback khi stop)
 */
K_TIMER_DEFINE(evt_timer, timer_isr, NULL);

/*
 * THREAD XỬ LÝ SỰ KIỆN (Worker Thread):
 * - Chạy trong Thread context: được phép ngủ (k_msleep), bị ngắt chen ngang thoải mái.
 */
static void thread_handler(void *p1, void *p2, void *p3)
{
	unsigned int n = 0;

	/* Cấu hình chân LED3 là ngõ ra, mức logic ban đầu là 0 (tắt) */
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/*
		 * k_sem_take(&evt_sem, K_FOREVER):
		 * - Nếu evt_sem == 0: Thread chuyển sang trạng thái WAITING/BLOCKED (nhường CPU cho task khác).
		 * - Khi timer_isr gọi k_sem_give(&evt_sem): evt_sem thành 1, thread lập tức tỉnh dậy,
		 *   giảm evt_sem về 0 và tiếp tục thực thi đoạn code bên dưới.
		 */
		k_sem_take(&evt_sem, K_FOREVER);
		n++;
		printf("[%6u ms] Su kien #%u -> thread xu ly\n", k_uptime_get_32(), n);

		/* Xử lý công việc tốn thời gian: Nháy LED 3 lần (mỗi chu kỳ tắt/bật mất 200ms -> tổng 600ms) */
		for (int i = 0; i < 3; i++) {
			gpio_pin_set_dt(&led3, 1); /* Bật LED */
			k_msleep(100);             /* Chờ 100ms */
			gpio_pin_set_dt(&led3, 0); /* Tắt LED */
			k_msleep(100);             /* Chờ 100ms */
		}
	}
}

/*
 * KHAI BÁO THREAD (Tự động tạo và kích hoạt khi boot):
 * - handler_tid: Tên định danh thread
 * - 1024: Kích thước vùng nhớ Stack (bytes)
 * - thread_handler: Hàm thực thi chính của thread
 * - NULL, NULL, NULL: 3 tham số truyền vào hàm (p1, p2, p3)
 * - 5: Mức ưu tiên (priority). Số càng nhỏ ưu tiên càng cao. Mức 5 là mức trung bình.
 * - 0: Cờ tùy chọn (options)
 * - 0: Thời gian delay trước khi bắt đầu chạy (0 = chạy ngay lập tức)
 */
K_THREAD_DEFINE(handler_tid, 1024, thread_handler, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	printf("Lab 2 - Demo 1: Binary semaphore (ISR -> thread)\n");

	/*
	 * k_timer_start:
	 * - evt_timer: Bộ đếm thời gian cần chạy
	 * - K_SECONDS(2): Hết hạn lần đầu sau 2 giây kể từ khi gọi hàm
	 * - K_SECONDS(2): Lặp lại định kỳ mỗi 2 giây một lần
	 */
	k_timer_start(&evt_timer, K_SECONDS(2), K_SECONDS(2));

	return 0;
}
