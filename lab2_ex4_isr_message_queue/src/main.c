/*
 * ==============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - BÀI THỰC HÀNH 2
 * BÀI TẬP 4: Thu thập dữ liệu từ ISR qua Message Queue
 * ==============================================================================
 *
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Sử dụng Message Queue để truyền dữ liệu an toàn từ ngắt (ISR) sang Thread.
 *    - Nắm vững việc dùng K_NO_WAIT trong ISR: ISR không được phép chờ (block),
 *      nếu hàng đợi đã đầy thì buộc phải chấp nhận drop (bỏ rơi) dữ liệu.
 *    - Sử dụng atomic_t để bảo vệ các biến đếm thống kê khỏi tranh chấp giữa ISR và Thread.
 *    - Phân tích hiện tượng tràn hàng đợi khi tốc độ sản sinh dữ liệu (200 ms/mẫu)
 *      nhanh hơn tốc độ tiêu thụ/xử lý (500 ms/mẫu ở phần a; 150 ms/mẫu ở phần b).
 *
 * 2. CẤU HÌNH THỜI GIAN:
 *    - Phần a: CONSUME_TIME_MS = 500 (250ms bật Y0, 250ms tắt Y0) -> Sẽ bị mất mẫu.
 *    - Phần b: CONSUME_TIME_MS = 150 (75ms bật Y0, 75ms tắt Y0) -> Không bị mất mẫu nào.
 * ==============================================================================
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/* Khai báo phần cứng LED từ Devicetree */
#define LED0_NODE DT_ALIAS(led0) /* LED3 - PE4: Báo hiệu khi có mẫu bị mất (drop) */
#define LED1_NODE DT_ALIAS(led1) /* Y0   - PE5: Nháy khi Consumer đang xử lý mẫu */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

/* Cấu trúc dữ liệu của mỗi mẫu (Sample) */
struct sample {
	uint32_t seq;  /* Số thứ tự gói tin tăng dần */
	uint32_t t_ms; /* Mốc thời gian tạo mẫu (ms) */
};

/*
 * KHỞI TẠO TĨNH MESSAGE QUEUE:
 * - Tên: sample_q
 * - Kích thước gói: sizeof(struct sample) (8 bytes)
 * - Dung lượng: 8 mẫu tối đa
 * - Alignment: 4 bytes
 */
K_MSGQ_DEFINE(sample_q, sizeof(struct sample), 8, 4);

/*
 * CÁC BỘ ĐẾM THỐNG KÊ (Dùng kiểu biến nguyên tử atomic_t để an toàn giữa ISR & Thread):
 */
static atomic_t n_sent = ATOMIC_INIT(0); /* Số mẫu ISR gửi thành công vào hàng đợi */
static atomic_t n_drop = ATOMIC_INIT(0); /* Số mẫu ISR bị rớt do hàng đợi đầy */
static atomic_t n_recv = ATOMIC_INIT(0); /* Số mẫu thread_consumer đã lấy ra xử lý */

/*
 * THỜI GIAN XỬ LÝ CỦA CONSUMER:
 * - Để chạy Phần a: Đặt là 500 ms (chậm hơn tốc độ timer 200 ms -> BỊ MẤT MẪU)
 * - Để chạy Phần b: Đổi sang 150 ms (nhanh hơn tốc độ timer 200 ms -> KHÔNG MẤT MẪU)
 */
#define CONSUME_TIME_MS 150

/*
 * HÀM NGẮT (ISR) CỦA TIMER: Chạy mỗi 200 ms
 */
static void sample_timer_isr(struct k_timer *timer)
{
	static uint32_t current_seq = 0;
	struct sample s;

	current_seq++;
	s.seq = current_seq;
	s.t_ms = k_uptime_get_32();

	/*
	 * k_msgq_put với K_NO_WAIT:
	 * BẮT BUỘC trong ngữ cảnh ngắt (ISR). Nếu hàng đợi đầy (đủ 8 mẫu chưa xử lý),
	 * hàm lập tức trả về lỗi (-ENOMSG / -EAGAIN) mà không chờ.
	 */
	int ret = k_msgq_put(&sample_q, &s, K_NO_WAIT);
	if (ret == 0) {
		/* TRƯỜNG HỢP THÀNH CÔNG (ret == 0):
		 * Hàng đợi còn chỗ trống, gói tin đã được copy vào an toàn */
		atomic_inc(&n_sent);
	} else {
		/* TRƯỜNG HỢP THẤT BẠI:
		 * Hàng đợi đã chật kín (8/8), gói tin bị vứt bỏ ra ngoài đất */
		atomic_inc(&n_drop);
	}
}
K_TIMER_DEFINE(sample_timer, sample_timer_isr, NULL);

/*
 * THREAD TIÊU THỤ / XỬ LÝ (thread_consumer):
 */
void fn_thread_consumer(void *p1, void *p2, void *p3)
{
	struct sample s;
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* Chờ lấy mẫu từ hàng đợi: Nếu rỗng thì thread tự ngủ chờ */
		k_msgq_get(&sample_q, &s, K_FOREVER);
		atomic_inc(&n_recv);

		/* Xử lý mẫu: Nháy đèn Y0 (sáng nửa chu kỳ, tắt nửa chu kỳ) */
		gpio_pin_set_dt(&y0, 1);
		k_msleep(CONSUME_TIME_MS / 2);
		gpio_pin_set_dt(&y0, 0);
		k_msleep(CONSUME_TIME_MS / 2);
	}
}

/*
 * THREAD GIÁM SÁT VÀ IN THỐNG KÊ (thread_monitor):
 */
void fn_thread_monitor(void *p1, void *p2, void *p3)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* Định kỳ mỗi 5 giây in 1 lần */
		k_msleep(5000);

		long sent = atomic_get(&n_sent);
		long recv = atomic_get(&n_recv);
		long drop = atomic_get(&n_drop);
		uint32_t in_q = k_msgq_num_used_get(&sample_q);

		printf("[%6u ms] GUI: %ld | NHAN: %ld | MAT: %ld | TRONG HANG DOI: %u/8\n", k_uptime_get_32(), sent,
		       recv, drop, in_q);

		/* Bật LED3 cảnh báo nếu đã có ít nhất một mẫu bị mất */
		if (drop > 0) {
			gpio_pin_set_dt(&led3, 1);
		}
	}
}

/* Khai báo 2 thread theo đúng tên trong đề */
K_THREAD_DEFINE(thread_consumer, 1024, fn_thread_consumer, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_monitor, 1024, fn_thread_monitor, NULL, NULL, NULL, 6, 0, 0);

int main(void)
{
	printf("=== BKIT ARM4: Lab 2 - Bai 4: Thu thap du lieu tu ISR qua Message Queue ===\n");
	printf("Cau hinh: Chu ky tao mau = 200 ms | Thoi gian xu ly = %d ms\n", CONSUME_TIME_MS);

	/* Khởi động timer chu kỳ 200 ms */
	k_timer_start(&sample_timer, K_MSEC(200), K_MSEC(200));

	return 0;
}
