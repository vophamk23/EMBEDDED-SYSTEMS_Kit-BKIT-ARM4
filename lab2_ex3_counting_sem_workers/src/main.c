/*
 * ==============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - BÀI THỰC HÀNH 2
 * BÀI TẬP 3: Chia sẻ 2 kênh Output cho 4 Worker bằng Counting Semaphore & Mutex
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Sử dụng Counting Semaphore để quản lý số lượng tài nguyên có giới hạn (Resource Pool).
 *      Ở đây: có 2 kênh output (Y0, Y1) phục vụ cho 4 worker -> tối đa 2 worker chạy cùng lúc.
 *    - Kết hợp Mutex để bảo vệ "Critical Section" (mảng trạng thái kênh ch_busy[2]):
 *      Khi một worker được cấp phép (qua semaphore), nó cần tìm chính xác kênh nào đang rảnh (0 hay 1).
 *      Mutex đảm bảo không có 2 worker nào cùng chọn trúng một kênh vật lý!
 *
 * 2. CẤU HÌNH CÁC WORKER:
 *    - W1: Làm việc 1000 ms, Nghỉ 500 ms
 *    - W2: Làm việc 1500 ms, Nghỉ 700 ms
 *    - W3: Làm việc 2000 ms, Nghỉ 300 ms
 *    - W4: Làm việc 2500 ms, Nghỉ 900 ms
 *
 * 3. QUY ĐỊNH PHẦN CỨNG:
 *    - Y0 (Kênh 0 - PE5), Y1 (Kênh 1 - PE6): Sáng khi có worker đang làm việc trên kênh đó.
 *    - LED3 (PE4): Sáng khi không còn kênh trống nào (cả 2 kênh đều bận), Tắt khi có ít nhất 1 kênh rảnh.
 * ==============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Khai báo phần cứng LED từ Devicetree */
#define LED0_NODE DT_ALIAS(led0) /* LED3 - PE4: Báo cả 2 kênh đều bận */
#define LED1_NODE DT_ALIAS(led1) /* Y0   - PE5: Kênh output 0 */
#define LED2_NODE DT_ALIAS(led2) /* Y1   - PE6: Kênh output 1 */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec channels[2] = {
	GPIO_DT_SPEC_GET(LED1_NODE, gpios), /* Kênh 0: Y0 */
	GPIO_DT_SPEC_GET(LED2_NODE, gpios)  /* Kênh 1: Y1 */
};
static const char *const ch_names[2] = {"Y0", "Y1"};

/*
 * KHỞI TẠO ĐỐI TƯỢNG ĐỒNG BỘ:
 * - pool_sem: Counting semaphore khởi tạo = 2, giới hạn tối đa = 2.
 *   Nhiệm vụ: Chặn không cho quá 2 worker truy cập cùng lúc.
 * - ch_mutex: Mutex bảo vệ việc tìm và đánh dấu kênh trong mảng ch_busy.
 */
K_SEM_DEFINE(pool_sem, 2, 2);
K_MUTEX_DEFINE(ch_mutex);

/* Trạng thái bận của từng kênh (true: đang có worker dùng, false: đang rảnh) */
static bool ch_busy[2] = {false, false};

/* Cấu trúc cấu hình cho từng worker */
struct worker_cfg {
	int id;         /* ID: 1, 2, 3, 4 */
	int work_ms;    /* Thời gian giữ kênh (ms) */
	int rest_ms;    /* Thời gian nghỉ ngơi (ms) */
};

static const struct worker_cfg cfg1 = {1, 1000, 500};
static const struct worker_cfg cfg2 = {2, 1500, 700};
static const struct worker_cfg cfg3 = {3, 2000, 300};
static const struct worker_cfg cfg4 = {4, 2500, 900};

/*
 * HÀM THỰC THI CỦA WORKER THREAD:
 * Dùng chung cho cả 4 worker, nhận cấu hình qua tham số p1.
 */
void thread_worker(void *p1, void *p2, void *p3)
{
	const struct worker_cfg *cfg = (const struct worker_cfg *)p1;

	while (1) {
		/* ==========================================================
		 * BƯỚC 1: XIN VÉ TRUY CẬP (Counting Semaphore)
		 * ========================================================== */
		/* Nếu cả 2 kênh đều đang bận (count == 0), worker sẽ bị BLOCK tại đây */
		k_sem_take(&pool_sem, K_FOREVER);

		/* ==========================================================
		 * BƯỚC 2: CHỌN KÊNH RẢNH (Critical Section được Mutex bảo vệ)
		 * ========================================================== */
		k_mutex_lock(&ch_mutex, K_FOREVER);
		int ch = -1;
		if (!ch_busy[0]) {
			ch = 0;
			ch_busy[0] = true;
		} else if (!ch_busy[1]) {
			ch = 1;
			ch_busy[1] = true;
		}

		/* LED3 sáng khi không còn kênh trống (pool_sem == 0), tắt khi có kênh trống */
		if (k_sem_count_get(&pool_sem) == 0) {
			gpio_pin_set_dt(&led3, 1);
		}
		k_mutex_unlock(&ch_mutex);

		/* ==========================================================
		 * BƯỚC 3: SỬ DỤNG KÊNH (Làm việc)
		 * ========================================================== */
		printf("[%6u ms] W%d dung kenh %s\n", k_uptime_get_32(), cfg->id, ch_names[ch]);

		/* Bật đèn của kênh tương ứng trong thời gian làm việc */
		gpio_pin_set_dt(&channels[ch], 1);
		k_msleep(cfg->work_ms);
		gpio_pin_set_dt(&channels[ch], 0);

		/* ==========================================================
		 * BƯỚC 4: TRẢ KÊNH VÀ GIẢI PHÓNG TÀI NGUYÊN
		 * ========================================================== */
		printf("[%6u ms] W%d tra kenh %s\n", k_uptime_get_32(), cfg->id, ch_names[ch]);

		k_mutex_lock(&ch_mutex, K_FOREVER);
		ch_busy[ch] = false;
		/* Vì vừa trả kênh nên chắc chắn có kênh trống -> Tắt LED3 */
		gpio_pin_set_dt(&led3, 0);
		k_mutex_unlock(&ch_mutex);

		/* Trả lại 1 vé cho pool_sem để đánh thức worker khác đang đợi */
		k_sem_give(&pool_sem);

		/* ==========================================================
		 * BƯỚC 5: NGHỈ NGƠI THEO CẤU HÌNH
		 * ========================================================== */
		k_msleep(cfg->rest_ms);
	}
}

/*
 * KHỞI TẠO 4 WORKER THREAD CÙNG ĐỘ ƯU TIÊN (Priority = 5)
 */
K_THREAD_DEFINE(worker1, 1024, thread_worker, (void *)&cfg1, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(worker2, 1024, thread_worker, (void *)&cfg2, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(worker3, 1024, thread_worker, (void *)&cfg3, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(worker4, 1024, thread_worker, (void *)&cfg4, NULL, NULL, 5, 0, 0);

int main(void)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&channels[0], GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&channels[1], GPIO_OUTPUT_INACTIVE);

	printf("=== BKIT ARM4: Lab 2 - Bai 3: Chia se 2 kenh output cho 4 worker ===\n");
	return 0;
}
