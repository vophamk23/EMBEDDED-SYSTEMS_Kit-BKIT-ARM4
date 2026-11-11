/*
 * ==============================================================================
 * BKIT ARM4 - Lab 2 - Demo 2: Counting Semaphore (Đếm và đệm sự kiện)
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI HỌC:
 *    - Hiểu cơ chế hoạt động của Counting Semaphore (semaphore đếm: giá trị từ 0 đến N).
 *    - Khác với Binary Semaphore (chỉ lưu 1 sự kiện), Counting Semaphore có khả năng
 *      "ghi nhớ" và "tích lũy" nhiều sự kiện chưa kịp xử lý.
 *    - Quan sát hiện tượng "Burst" (bùng nổ sự kiện) và mất mát dữ liệu (event loss):
 *        + Nguồn phát (Timer ISR): Phát ra sự kiện rất nhanh (mỗi 300 ms).
 *        + Bên xử lý (Consumer Thread): Xử lý khá chậm (mất 1000 ms mỗi sự kiện).
 *        + Semaphore đóng vai trò như một bộ đệm chứa tối đa EVT_LIMIT (5) sự kiện.
 *        + Khi bộ đệm đầy (count == 5), các sự kiện mới sinh ra sẽ bị BỎ RƠI (lost).
 *
 * 2. CÁC HÀM VÀ MACRO ZEPHYR QUAN TRỌNG:
 *    - K_SEM_DEFINE(name, initial_count, count_limit): Khởi tạo semaphore với giới hạn tối đa N.
 *    - k_sem_count_get(sem): Đọc giá trị đếm hiện tại của semaphore (số sự kiện đang chờ).
 *    - k_sem_give(sem): Tăng bộ đếm lên 1 (nếu chưa đạt count_limit).
 *    - k_sem_take(sem, timeout): Giảm bộ đếm đi 1; nếu đếm = 0 thì thread sẽ chờ (block).
 *    - k_timer_stop(timer): Dừng bộ đếm thời gian.
 * ==============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Giới hạn số lượng sự kiện tối đa mà semaphore có thể tích lũy (bộ đệm hàng đợi tín hiệu) */
#define EVT_LIMIT 5

/* Cấu hình các chân LED từ Device Tree */
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);   /* LED báo trạng thái đang xử lý */
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios); /* LED báo trạng thái Timer đang phát burst */

/*
 * KHỞI TẠO COUNTING SEMAPHORE:
 * - Tên: evt_count
 * - Giá trị ban đầu: 0 (chưa có sự kiện nào)
 * - Giá trị tối đa: EVT_LIMIT (5) (chỉ lưu tối đa 5 sự kiện chưa xử lý)
 */
K_SEM_DEFINE(evt_count, 0, EVT_LIMIT);

/* Biến đếm toàn cục dùng volatile vì được truy xuất từ cả ISR và Thread */
static volatile unsigned int produced; /* Tổng số sự kiện đã được Timer tạo ra */
static volatile unsigned int lost;     /* Số sự kiện bị mất do bộ đếm semaphore đã đầy */

/*
 * HÀM NGẮT TIMER (Bên phát - Producer):
 * - Được gọi mỗi 300 ms trong đợt burst.
 */
static void timer_isr(struct k_timer *timer)
{
	produced++;

	/* Kiểm tra xem semaphore đã đạt mức tối đa chưa */
	if (k_sem_count_get(&evt_count) >= EVT_LIMIT) {
		lost++; /* Semaphore đã đầy 5/5: gọi k_sem_give sẽ bị bỏ qua -> sự kiện bị mất! */
	}

	/* Tăng bộ đếm semaphore nếu chưa đạt giới hạn */
	k_sem_give(&evt_count);
}
K_TIMER_DEFINE(evt_timer, timer_isr, NULL);

/*
 * THREAD TIÊU THỤ (Bên nhận & xử lý - Consumer):
 * - Tốc độ xử lý: 700ms bật LED + 300ms tắt LED = 1000ms (1 giây) cho mỗi sự kiện.
 */
static void thread_consumer(void *p1, void *p2, void *p3)
{
	unsigned int done = 0;

	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/*
		 * k_sem_take(&evt_count, K_FOREVER):
		 * - Nếu evt_count > 0: giảm evt_count đi 1 và chạy tiếp ngay lập tức.
		 * - Nếu evt_count == 0: thread bị block và nhường CPU cho đến khi có sự kiện mới.
		 */
		k_sem_take(&evt_count, K_FOREVER);
		done++;

		/* In thống kê: số đã xử lý, số còn tồn đọng trong sem, tổng đã tạo, tổng bị mất */
		printf("  xu ly #%2u | con cho: %u | da tao: %u | bi mat: %u\n",
		       done, k_sem_count_get(&evt_count), produced, lost);

		/* Giả lập công việc nặng tốn 1000ms: Bật đèn Y0 sáng trong lúc đang xử lý */
		gpio_pin_set_dt(&y0, 1);    /* Y0 sáng: Bắt đầu xử lý */
		k_msleep(700);              /* Tốn 700 ms */
		gpio_pin_set_dt(&y0, 0);    /* Tắt Y0 */
		k_msleep(300);              /* Tốn thêm 300 ms nghỉ */
	}
}
K_THREAD_DEFINE(consumer_tid, 1024, thread_consumer, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	printf("Lab 2 - Demo 2: Counting semaphore (limit = %d)\n", EVT_LIMIT);

	while (1) {
		printf("--- Burst: 15 su kien, moi 300 ms ---\n");
		gpio_pin_set_dt(&led3, 1);  /* Bật LED3: Đánh dấu đang trong giai đoạn phát sự kiện dồn dập */

		/*
		 * Khởi động timer: bắn ra 1 sự kiện mỗi 300 ms
		 * Trong 4500 ms sẽ bắn ra 15 sự kiện (4500 / 300 = 15).
		 */
		k_timer_start(&evt_timer, K_MSEC(300), K_MSEC(300));
		k_msleep(4500 + 100);       /* Ngủ chờ timer bắn đủ 15 lần */

		/* Dừng phát sự kiện */
		k_timer_stop(&evt_timer);
		gpio_pin_set_dt(&led3, 0);  /* Tắt LED3 */

		/*
		 * Sau khi timer dừng, consumer thread vẫn tiếp tục rút các sự kiện
		 * đang được lưu trong semaphore ra để xử lý tiếp cho đến khi về 0.
		 * Ta cho main ngủ 10 giây để chờ consumer hoàn tất xử lý hết.
		 */
		k_msleep(10000);
	}
	return 0;
}
