/*
 * ==============================================================================
 * BKIT ARM4 - Lab 2 - Demo 4: Message Queue (Giao tiếp dữ liệu Producer -> Consumer)
 * ==============================================================================
 * 
 * 1. MỤC TIÊU BÀI HỌC:
 *    - Hiểu cơ chế Hàng đợi thông điệp (Message Queue) trong RTOS.
 *    - Phân biệt giữa Semaphore và Message Queue:
 *        + Semaphore: Chỉ mang ý nghĩa TÍN HIỆU (báo có/không hoặc đếm), KHÔNG truyền được dữ liệu.
 *        + Message Queue: Truyền được cả GÓI DỮ LIỆU hoàn chỉnh (struct, payload) an toàn
 *          giữa các thread theo nguyên tắc FIFO (vào trước - ra trước).
 *    - Áp dụng mô hình kinh điển Producer - Consumer:
 *        + Thread "Producer": Tạo ra các lệnh điều khiển LED và gửi vào hàng đợi qua k_msgq_put().
 *        + Thread "Consumer": Đọc lệnh từ hàng đợi qua k_msgq_get(), giải mã và bật tắt LED phần cứng.
 *    - Cơ chế tự động khóa/chặn (Blocking) của Message Queue:
 *        + Nếu hàng đợi ĐẦY: k_msgq_put() sẽ cho Producer ngủ chờ đến khi có chỗ trống.
 *        + Nếu hàng đợi RỖNG: k_msgq_get() sẽ cho Consumer ngủ chờ đến khi có dữ liệu mới.
 *
 * 2. CÁC HÀM VÀ MACRO ZEPHYR QUAN TRỌNG:
 *    - K_MSGQ_DEFINE(q_name, msg_size, max_msgs, align): Khởi tạo tĩnh một message queue.
 *    - k_msgq_put(msgq, data_ptr, timeout): Gửi một thông điệp vào cuối hàng đợi (copy theo giá trị).
 *    - k_msgq_get(msgq, data_ptr, timeout): Rút thông điệp ở đầu hàng đợi ra.
 *    - k_msgq_num_used_get(msgq): Lấy số lượng thông điệp hiện đang nằm chờ trong hàng đợi.
 * ==============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/*
 * ĐỊNH NGHĨA CẤU TRÚC GÓI TIN (Message Payload):
 * Mỗi lệnh điều khiển gồm:
 * - led: Chỉ số của đèn LED cần bật (0 = LED3, 1 = Y0, 2 = Y1)
 * - ms: Thời gian bật đèn tính bằng mili-giây
 */
struct led_cmd {
	uint8_t led;
	uint16_t ms;
};

/*
 * KHỞI TẠO TĨNH MESSAGE QUEUE:
 * - cmd_q: Tên của hàng đợi
 * - sizeof(struct led_cmd): Kích thước của mỗi phần tử (3 bytes, được align lên 4 bytes)
 * - 4: Số lượng thông điệp tối đa hàng đợi có thể chứa cùng lúc (dung lượng = 4)
 * - 4: Yêu cầu căn chỉnh địa chỉ bộ nhớ (alignment: 4 bytes)
 */
K_MSGQ_DEFINE(cmd_q, sizeof(struct led_cmd), 4, 4);

/* Mảng chứa cấu hình chân cho 3 đèn LED */
static const struct gpio_dt_spec leds[3] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios), /* LED3 (xanh lá trên bo Discovery) */
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios), /* Y0 (cam) */
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios), /* Y1 (đỏ) */
};
static const char *const names[3] = { "LED3", "Y0", "Y1" };

/*
 * THREAD PHÁT LỆNH (PRODUCER):
 * - Nhiệm vụ: Tạo ra các hiệu ứng ánh sáng (mẫu nháy) và đẩy lệnh vào hàng đợi.
 */
static void thread_producer(void *p1, void *p2, void *p3)
{
	/* Mẫu hiệu ứng chạy đuổi: LED3 -> Y0 -> Y1 -> Y0 -> LED3 -> Y1 (sáng 1s) */
	static const struct led_cmd pattern[] = {
		{0, 300}, {1, 300}, {2, 300}, {1, 300}, {0, 300}, {2, 1000},
	};

	while (1) {
		for (int i = 0; i < ARRAY_SIZE(pattern); i++) {
			/*
			 * k_msgq_put:
			 * - Đẩy địa chỉ của pattern[i] vào cmd_q (dữ liệu được copy vào buffer của queue).
			 * - K_FOREVER: Nếu hàng đợi đã đầy 4/4 phần tử, producer sẽ tự động bị BLOCK (ngủ),
			 *   nhường CPU cho consumer lấy bớt lệnh ra rồi mới gửi tiếp.
			 */
			k_msgq_put(&cmd_q, &pattern[i], K_FOREVER);

			printf("[P] gui lenh %d (%s) | trong hang doi: %u/4\n",
			       i, names[pattern[i].led], k_msgq_num_used_get(&cmd_q));
		}

		/* Nghỉ 2 giây trước khi gửi lại chu kỳ tiếp theo */
		k_msleep(2000);
	}
}

/*
 * THREAD THỰC THI LỆNH (CONSUMER / LED DRIVER):
 * - Nhiệm vụ: Lấy từng lệnh ra khỏi hàng đợi và điều khiển chân phần cứng.
 */
static void thread_consumer(void *p1, void *p2, void *p3)
{
	struct led_cmd cmd;

	/* Khởi tạo cấu hình Output cho cả 3 chân LED */
	for (int i = 0; i < 3; i++) {
		gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
	}

	while (1) {
		/*
		 * k_msgq_get:
		 * - Rút phần tử đầu tiên (đến sớm nhất - FIFO) trong cmd_q ra và lưu vào biến cmd.
		 * - K_FOREVER: Nếu hàng đợi rỗng (0/4), consumer sẽ tự động bị BLOCK (ngủ),
		 *   tiết kiệm 100% CPU cho đến khi có lệnh mới được producer đẩy vào.
		 */
		k_msgq_get(&cmd_q, &cmd, K_FOREVER);

		printf("    [C] thuc thi: %s sang %u ms\n", names[cmd.led], cmd.ms);

		/* Thực thi lệnh: Bật đèn tương ứng trong đúng số ms chỉ định */
		gpio_pin_set_dt(&leds[cmd.led], 1); /* Bật sáng */
		k_msleep(cmd.ms);                   /* Giữ sáng */
		gpio_pin_set_dt(&leds[cmd.led], 0); /* Tắt đèn */
	}
}

/* Khởi tạo tự động 2 thread với mức ưu tiên bằng nhau (Priority = 7) */
K_THREAD_DEFINE(producer_tid, 1024, thread_producer, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(consumer_tid, 1024, thread_consumer, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	printf("Lab 2 - Demo 4: Message queue (Producer -> Consumer)\n");
	return 0;
}
