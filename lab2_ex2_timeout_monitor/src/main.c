/*
 * ==============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - BÀI THỰC HÀNH 2
 * BÀI TẬP 2: Giám sát tín hiệu có Timeout bằng Binary Semaphore
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Sử dụng Binary Semaphore để nhận tín hiệu sự kiện từ ngắt phần cứng/ISR sang Thread.
 *    - Sử dụng tham số timeout trong k_sem_take() để phát hiện sự cố mất tín hiệu / treo hệ thống.
 *    - Xử lý giá trị trả về của k_sem_take():
 *        + 0: Lấy semaphore thành công trong thời gian cho phép (tín hiệu bình thường).
 *        + -EAGAIN: Hết thời gian chờ (timeout) mà không có tín hiệu (mất tín hiệu).
 *
 * 2. MÔ TẢ KỊCH BẢN THỬ NGHIỆM:
 *    - k_timer phát tín hiệu ngắt theo chu kỳ: 1 s -> 1 s -> 4 s rồi lặp lại.
 *    - Thread thread_monitor chờ tối đa 2000 ms (2 s):
 *        + Lần 1 (sau 1 s): Có tín hiệu trước 2s -> Nhận tín hiệu OK -> Nháy Y0 hai lần.
 *        + Lần 2 (sau 1 s): Có tín hiệu trước 2s -> Nhận tín hiệu OK -> Nháy Y0 hai lần.
 *        + Lần 3 (sau 4 s): Chờ quá 2s vẫn chưa có tín hiệu -> TIMEOUT! -> Đảo trạng thái LED3.
 *          Sau đó timer bắn lúc 4s, tín hiệu tồn đọng trong semaphore sẽ được nhận ở chu kỳ kế.
 * ==============================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Khai báo phần cứng LED từ Devicetree */
#define LED0_NODE DT_ALIAS(led0) /* LED3 - PE4: Đèn cảnh báo Timeout */
#define LED1_NODE DT_ALIAS(led1) /* Y0   - PE5: Đèn báo nhận tín hiệu bình thường */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/*
 * KHỞI TẠO BINARY SEMAPHORE:
 * - sig_sem: Ban đầu = 0, tối đa = 1
 */
K_SEM_DEFINE(sig_sem, 0, 1);

/* Mảng chu kỳ phát tín hiệu: 1s -> 1s -> 4s */
static const int delay_pattern_ms[] = {1000, 1000, 4000};
static int pattern_idx = 0;

/*
 * HÀM NGẮT (ISR) CỦA TIMER:
 * - Nguyên tắc: KHÔNG được gọi printf, KHÔNG được bật tắt LED, KHÔNG được sleep.
 * - Chỉ phát tín hiệu (give semaphore) và đặt lại timer cho lần kế tiếp.
 */
static void sig_timer_isr(struct k_timer *timer)
{
	/* Phát tín hiệu cho thread */
	k_sem_give(&sig_sem);

	/* Tự khởi động lại timer với chu kỳ kế tiếp trong mảng pattern (chế độ one-shot) */
	k_timer_start(timer, K_MSEC(delay_pattern_ms[pattern_idx]), K_NO_WAIT);
	pattern_idx = (pattern_idx + 1) % 3;
}

K_TIMER_DEFINE(sig_timer, sig_timer_isr, NULL);

/*
 * THREAD GIÁM SÁT TÍN HIỆU (thread_monitor):
 */
void fn_thread_monitor(void *p1, void *p2, void *p3)
{
	/* Cấu hình các chân LED ban đầu tắt */
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0,   GPIO_OUTPUT_INACTIVE);

	while (1) {
		/*
		 * k_sem_take(&sig_sem, K_MSEC(2000)):
		 * Chờ tối đa 2000 ms.
		 * - Nếu có tín hiệu trong vòng 2000 ms: trả về 0.
		 * - Nếu quá 2000 ms mà không có ai give: trả về -EAGAIN.
		 */
		int ret = k_sem_take(&sig_sem, K_MSEC(2000));

		if (ret == 0) {
			/* TRƯỜNG HỢP 1: Nhận tín hiệu thành công */
			printf("[%6u ms] Nhan tin hieu\n", k_uptime_get_32());

			/* Nháy Y0 hai lần (mỗi lần 100 ms sáng / 100 ms tắt) */
			for (int i = 0; i < 2; i++) {
				gpio_pin_set_dt(&y0, 1);
				k_msleep(100);
				gpio_pin_set_dt(&y0, 0);
				k_msleep(100);
			}
		} else if (ret == -EAGAIN) {
			/* TRƯỜNG HỢP 2: Quá 2 giây không nhận được tín hiệu -> Timeout */
			printf("[%6u ms] TIMEOUT - mat tin hieu!\n", k_uptime_get_32());

			/* Đảo trạng thái đèn cảnh báo LED3 */
			gpio_pin_toggle_dt(&led3);
		}
	}
}

/* Khởi tạo thread giám sát theo đúng tên thread_monitor trong đề */
K_THREAD_DEFINE(thread_monitor, 1024, fn_thread_monitor, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	printf("=== BKIT ARM4: Lab 2 - Bai 2: Giam sat tin hieu co Timeout ===\n");

	/* Khởi động chu kỳ đầu tiên của timer (1000 ms) */
	k_timer_start(&sig_timer, K_MSEC(delay_pattern_ms[0]), K_NO_WAIT);
	pattern_idx = 1;

	return 0;
}
