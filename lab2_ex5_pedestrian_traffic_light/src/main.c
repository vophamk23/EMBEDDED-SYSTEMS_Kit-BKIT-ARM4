/*
 * ==============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - BÀI THỰC HÀNH 2
 * BÀI TẬP 5: Đèn giao thông có nút xin qua đường (Bài tổng hợp)
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Tích hợp toàn diện các cơ chế đồng bộ và IPC của Zephyr RTOS:
 *        + Binary Semaphore có Timeout (k_sem_take với thời gian chờ giới hạn).
 *        + Reset Semaphore (k_sem_reset) để xóa các tín hiệu quá hạn không mong muốn.
 *        + Message Queue (k_msgq) để phục vụ kiến trúc GHI LOG TẬP TRUNG (Centralized Logger).
 *        + Quản lý đa tác vụ với nhiều Thread và mức ưu tiên khác nhau.
 *
 * 2. CÁC THÀNH PHẦN HỆ THỐNG:
 *    - thread_traffic_light (Priority 5):
 *        + Đèn ĐỎ (Y0 sáng): 7 giây.
 *        + Đèn XANH (Y1 sáng): 5 giây (nhưng sáng tối thiểu 2 giây; 3 giây sau nếu có
 *          người bấm nút thì kết thúc sớm để nhường đường).
 *        + Đèn VÀNG (Cả Y0 và Y1 cùng sáng): 2 giây.
 *    - k_timer giả lập nút bấm (ISR):
 *        + Bấm lần đầu ở giây thứ 10, sau đó cách nhau 14s, 8s, 17s rồi lặp lại.
 *        + Gửi tín hiệu k_sem_give(&ped_sem) và đẩy sự kiện vào log_q.
 *    - thread_logger (Priority 10 - Thấp nhất):
 *        + Là THREAD DUY NHẤT trong toàn bộ hệ thống được phép gọi printf().
 *        + Giúp tránh tranh chấp cổng UART, không làm gián đoạn các thread thời gian thực.
 *    - thread_led_blinky (Priority 7):
 *        + Nháy LED3 chu kỳ 1s bật / 1s tắt (Heartbeat báo hệ thống đang sống bình thường).
 * ==============================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Khai báo phần cứng LED từ Devicetree */
#define LED0_NODE DT_ALIAS(led0) /* LED3 - PE4: Heartbeat Blinky */
#define LED1_NODE DT_ALIAS(led1) /* Y0   - PE5: Đèn ĐỎ */
#define LED2_NODE DT_ALIAS(led2) /* Y1   - PE6: Đèn XANH */

static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec y1   = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

/* Định nghĩa các pha đèn giao thông */
#define PHASE_RED    0
#define PHASE_GREEN  1
#define PHASE_YELLOW 2

static const char *const phase_names[] = {"DO", "XANH", "VANG"};

/* Định nghĩa các loại sự kiện cho Logger */
#define EVT_PHASE_CHANGE    1  /* Sự kiện chuyển pha đèn */
#define EVT_PED_REQUEST     2  /* Sự kiện người đi bộ bấm nút */
#define EVT_GREEN_SHORTENED 3  /* Sự kiện rút ngắn pha xanh */

/* Cấu trúc sự kiện gửi qua Message Queue */
struct evt {
	uint8_t  type;   /* Loại sự kiện: EVT_... */
	uint8_t  phase;  /* Pha đèn tại thời điểm xảy ra sự kiện */
	uint32_t t_ms;   /* Mốc thời gian hệ thống (ms) */
};

/* Khởi tạo Message Queue chứa tối đa 16 sự kiện ghi log */
K_MSGQ_DEFINE(log_q, sizeof(struct evt), 16, 4);

/* Khởi tạo Binary Semaphore cho nút bấm người đi bộ */
K_SEM_DEFINE(ped_sem, 0, 1);

/* Hàm tiện ích: Đóng gói và gửi sự kiện vào hàng đợi log_q với K_NO_WAIT */
static inline void send_log_evt(uint8_t type, uint8_t phase)
{
	struct evt e = {
		.type  = type,
		.phase = phase,
		.t_ms  = k_uptime_get_32(),
	};
	k_msgq_put(&log_q, &e, K_NO_WAIT);
}

/*
 * MÔ PHỎNG NÚT XIN QUA ĐƯỜNG BẰNG TIMER (ISR):
 * - Lần đầu sau 10 giây (10000 ms)
 * - Sau đó cách nhau lần lượt: 14 s, 8 s, 17 s rồi lặp lại
 */
static const int ped_intervals_ms[] = {14000, 8000, 17000};
static int ped_idx = 0;

static void ped_timer_isr(struct k_timer *timer)
{
	/* Phát tín hiệu cho thread đèn */
	k_sem_give(&ped_sem);

	/* Gửi thông báo có sự kiện bấm nút vào hàng đợi log */
	struct evt e = {
		.type  = EVT_PED_REQUEST,
		.phase = 0,
		.t_ms  = k_uptime_get_32(),
	};
	k_msgq_put(&log_q, &e, K_NO_WAIT);

	/* Tự lên lịch cho lần bấm nút giả lập tiếp theo */
	k_timer_start(timer, K_MSEC(ped_intervals_ms[ped_idx]), K_NO_WAIT);
	ped_idx = (ped_idx + 1) % 3;
}
K_TIMER_DEFINE(ped_timer, ped_timer_isr, NULL);

/*
 * THREAD ĐIỀU KHIỂN ĐÈN GIAO THÔNG (thread_traffic_light):
 */
void fn_thread_traffic_light(void *p1, void *p2, void *p3)
{
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y1, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/* ==========================================================
		 * 1. PHA ĐỎ: Y0 BẬT, Y1 TẮT (Kéo dài 7 giây)
		 * ========================================================== */
		gpio_pin_set_dt(&y0, 1);
		gpio_pin_set_dt(&y1, 0);
		send_log_evt(EVT_PHASE_CHANGE, PHASE_RED);
		k_msleep(7000);

		/* ==========================================================
		 * 2. PHA XANH: Y0 TẮT, Y1 BẬT (5 giây)
		 * ========================================================== */
		gpio_pin_set_dt(&y0, 0);
		gpio_pin_set_dt(&y1, 1);
		send_log_evt(EVT_PHASE_CHANGE, PHASE_GREEN);

		/* Quy tắc: Luôn sáng ít nhất 2 giây */
		k_msleep(2000);

		/*
		 * XÓA CÁC YÊU CẦU CŨ:
		 * Nếu người đi bộ bấm nút lúc đèn đang Đỏ hoặc trong 2 giây đầu,
		 * ta bỏ qua yêu cầu đó bằng k_sem_reset() để chỉ đón nhận yêu cầu mới.
		 */
		k_sem_reset(&ped_sem);

		/*
		 * Chờ yêu cầu mới trong 3 giây còn lại (5s - 2s = 3s = 3000 ms):
		 * - Nếu có yêu cầu: ret == 0 -> Rút ngắn pha xanh, chuyển ngay sang Vàng.
		 * - Nếu không có ai bấm: hết 3s hàm trả về -EAGAIN -> Chuyển sang Vàng bình thường.
		 */
		int ret = k_sem_take(&ped_sem, K_MSEC(3000));
		if (ret == 0) {
			send_log_evt(EVT_GREEN_SHORTENED, PHASE_GREEN);
		}

		/* ==========================================================
		 * 3. PHA VÀNG: CẢ Y0 VÀ Y1 ĐỀU BẬT (Kéo dài 2 giây)
		 * ========================================================== */
		gpio_pin_set_dt(&y0, 1);
		gpio_pin_set_dt(&y1, 1);
		send_log_evt(EVT_PHASE_CHANGE, PHASE_YELLOW);
		k_msleep(2000);
	}
}

/*
 * THREAD GHI LOG TẬP TRUNG (thread_logger):
 * - Có mức ưu tiên thấp nhất hệ thống (Priority = 10) để không làm trễ các tác vụ thời gian thực.
 * - Là nơi duy nhất in thông tin ra cổng Serial Console.
 */
void fn_thread_logger(void *p1, void *p2, void *p3)
{
	struct evt e;

	while (1) {
		k_msgq_get(&log_q, &e, K_FOREVER);

		switch (e.type) {
		case EVT_PHASE_CHANGE:
			printf("[%6u ms] [PHA DEN] Chuyen sang: %s\n", e.t_ms, phase_names[e.phase]);
			break;
		case EVT_PED_REQUEST:
			printf("[%6u ms] [NUT BAM] Nguoi di bo bam xin qua duong!\n", e.t_ms);
			break;
		case EVT_GREEN_SHORTENED:
			printf("[%6u ms] [UU TIEN] Rut ngan pha XANH de nhuong duong!\n", e.t_ms);
			break;
		default:
			break;
		}
	}
}

/*
 * THREAD NHÁY LED HEARTBEAT (thread_led_blinky):
 * - LED3 nháy 1s sáng / 1s tắt báo hiệu toàn bộ hệ thống đang hoạt động tốt.
 */
void fn_thread_led_blinky(void *p1, void *p2, void *p3)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

	while (1) {
		gpio_pin_set_dt(&led3, 1);
		k_msleep(1000);
		gpio_pin_set_dt(&led3, 0);
		k_msleep(1000);
	}
}

/*
 * KHỞI TẠO CÁC THREAD VỚI ĐỘ ƯU TIÊN PHÙ HỢP:
 * - thread_traffic_light: Priority 5 (Tác vụ điều khiển chính)
 * - thread_led_blinky:   Priority 7 (Báo hiệu hệ thống)
 * - thread_logger:       Priority 10 (Ghi log - ưu tiên thấp nhất)
 */
K_THREAD_DEFINE(thread_traffic_light, 1024, fn_thread_traffic_light, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_led_blinky,    1024, fn_thread_led_blinky,    NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(thread_logger,        1024, fn_thread_logger,        NULL, NULL, NULL, 10, 0, 0);

int main(void)
{
	/* Khởi động mô phỏng nút bấm lần đầu tiên sau 10 giây (10000 ms) */
	k_timer_start(&ped_timer, K_MSEC(10000), K_NO_WAIT);
	ped_idx = 0;

	return 0;
}
