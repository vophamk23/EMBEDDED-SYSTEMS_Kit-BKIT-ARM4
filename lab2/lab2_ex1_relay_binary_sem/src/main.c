/*
 * ==============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - BÀI THỰC HÀNH 2
 * BÀI TẬP 1: Chạy tiếp sức ba đèn bằng Binary Semaphore
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Sử dụng các Binary Semaphore để ép thứ tự thực thi tuần tự, chặt chẽ
 *      giữa các Thread chạy song song.
 *    - Không dùng biến cờ toàn cục (như int turn) hay vòng lặp bận (busy-wait).
 *    - Không dùng k_msleep() để canh giờ khởi động giữa các thread.
 *
 * 2. NGUYÊN LÝ HOẠT ĐỘNG:
 *    - Ba thread (thread_led3, thread_y0, thread_y1) có cùng mức ưu tiên (Priority 5).
 *    - Ba binary semaphore: sem_led3, sem_y0, sem_y1.
 *    - Ban đầu:
 *        + sem_led3 khởi tạo = 1 (để thread_led3 chạy đầu tiên).
 *        + sem_y0 và sem_y1 khởi tạo = 0 (phải chờ).
 *    - Chu trình luân chuyển (relay race):
 *        + thread_led3: take(sem_led3) -> Bật LED3 500ms -> Tắt -> give(sem_y0)
 *        + thread_y0:   take(sem_y0)   -> Bật Y0   500ms -> Tắt -> give(sem_y1)
 *        + thread_y1:   take(sem_y1)   -> Bật Y1   500ms -> Tắt -> give(sem_led3)
 *        -> Lặp lại vô hạn: LED3 -> Y0 -> Y1 -> LED3 -> ...
 *    - Đảm bảo tại mọi thời điểm chỉ có ĐÚNG 1 đèn sáng.
 * ==============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Khai báo phần cứng LED từ Devicetree */
#define LED0_NODE DT_ALIAS(led0) /* LED3 - PE4 */
#define LED1_NODE DT_ALIAS(led1) /* Y0   - PE5 */
#define LED2_NODE DT_ALIAS(led2) /* Y1   - PE6 */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec y1   = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

/*
 * KHỞI TẠO 3 BINARY SEMAPHORE:
 * - sem_led3: ban đầu = 1 để thread_led3 được phép chạy ngay khi hệ thống khởi động.
 * - sem_y0:   ban đầu = 0 để thread_y0 phải chờ đến khi thread_led3 give.
 * - sem_y1:   ban đầu = 0 để thread_y1 phải chờ đến khi thread_y0 give.
 */
K_SEM_DEFINE(sem_led3, 1, 1);
K_SEM_DEFINE(sem_y0,   0, 1);
K_SEM_DEFINE(sem_y1,   0, 1);

/* Thời gian mỗi đèn sáng (mili-giây) */
#define BLINK_TIME_MS 500

/*
 * THREAD ĐIỀU KHIỂN LED3:
 */
void fn_thread_led3(void *p1, void *p2, void *p3)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* Chờ đến lượt của mình */
		k_sem_take(&sem_led3, K_FOREVER);

		/* In log đúng định dạng yêu cầu: [thời điểm] TÊN_ĐÈN */
		printf("[%6u ms] LED3\n", k_uptime_get_32());

		/* Bật đèn sáng trong 500 ms */
		gpio_pin_set_dt(&led3, 1);
		k_msleep(BLINK_TIME_MS);
		gpio_pin_set_dt(&led3, 0);

		/* Chuyển lượt tiếp theo cho đèn Y0 */
		k_sem_give(&sem_y0);
	}
}

/*
 * THREAD ĐIỀU KHIỂN Y0:
 */
void fn_thread_y0(void *p1, void *p2, void *p3)
{
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* Chờ đến lượt từ LED3 */
		k_sem_take(&sem_y0, K_FOREVER);

		printf("[%6u ms] Y0\n", k_uptime_get_32());

		gpio_pin_set_dt(&y0, 1);
		k_msleep(BLINK_TIME_MS);
		gpio_pin_set_dt(&y0, 0);

		/* Chuyển lượt tiếp theo cho đèn Y1 */
		k_sem_give(&sem_y1);
	}
}

/*
 * THREAD ĐIỀU KHIỂN Y1:
 */
void fn_thread_y1(void *p1, void *p2, void *p3)
{
	gpio_pin_configure_dt(&y1, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* Chờ đến lượt từ Y0 */
		k_sem_take(&sem_y1, K_FOREVER);

		printf("[%6u ms] Y1\n", k_uptime_get_32());

		gpio_pin_set_dt(&y1, 1);
		k_msleep(BLINK_TIME_MS);
		gpio_pin_set_dt(&y1, 0);

		/* Chuyển lượt quay trở lại cho đèn LED3 để khép kín chu trình */
		k_sem_give(&sem_led3);
	}
}

/*
 * KHAI BÁO CẢ 3 THREAD VỚI CÙNG ĐỘ ƯU TIÊN (Priority = 5), STACK 1024 BYTES
 * Tên thread được đặt đúng theo yêu cầu trong đề: thread_led3, thread_y0, thread_y1.
 */
K_THREAD_DEFINE(thread_led3, 1024, fn_thread_led3, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_y0,   1024, fn_thread_y0,   NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_y1,   1024, fn_thread_y1,   NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	printf("=== BKIT ARM4: Lab 2 - Bai 1: Tiep suc 3 den bang Binary Semaphore ===\n");
	return 0;
}
