/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * DEMO 2: BỘ LỌC PHẦN CỨNG CAN (HARDWARE ACCEPTANCE FILTERING)
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI DEMO:
 *    - Hiểu sâu nguyên lý bộ lọc phần cứng (bxCAN Filter Banks) trên vi điều khiển:
 *      bxCAN dùng cơ chế ID và MASK để so khớp từng bit địa chỉ.
 *      Công thức lọc: (ID_nhận & mask) == (filter_id & mask).
 *      Các bit có mask = 1 bắt buộc phải khớp hoàn toàn; mask = 0 là "don't care" (bỏ qua).
 *    - Phân biệt 2 dạng ID:
 *      + Standard ID (11-bit, CAN 2.0A): mask chuẩn là CAN_STD_ID_MASK (0x7FF).
 *      + Extended ID (29-bit, CAN 2.0B): cờ CAN_FILTER_IDE, mask chuẩn CAN_EXT_ID_MASK (0x1FFFFFFF).
 *    - Tìm hiểu cơ chế Remote Transmission Request (RTR):
 *      Khung yêu cầu truyền dữ liệu (không chứa payload), nhận biết qua cờ CAN_FRAME_RTR.
 *    - So sánh 2 phương thức nhận frame trong Zephyr:
 *      1. can_add_rx_filter(): Gắn callback thực thi trực tiếp trong ngữ cảnh ngắt (ISR).
 *         -> BẮT BUỘC: Không gọi hàm blocking, không printf, đẩy dữ liệu sang thread bằng
 *            k_msgq_put(..., K_NO_WAIT).
 *      2. can_add_rx_filter_msgq(): Driver tự động đưa frame vào hàng đợi, thread đọc bằng k_msgq_get.
 *
 * 2. CẤU HÌNH BỘ LỌC TRONG BÀI:
 *    - Bộ lọc A: ID chuẩn 0x100, MASK 0x7F0 (bỏ qua 4 bit thấp) -> Nhận dải 0x100 .. 0x10F.
 *                Sử dụng Callback (ISR) -> chuyển qua evt_q sang thread_log_a -> đảo LED Y0 (PE5).
 *    - Bộ lọc B: ID mở rộng 0x18FF0001, MASK 0x1FFFFFFF (khớp tuyệt đối 29 bit), cờ CAN_FILTER_IDE.
 *                Sử dụng Message Queue (ext_msgq) -> thread_rx_b -> đảo LED Y1 (PE6).
 *    - Các frame khác (như 0x110, 0x200, 0x18FF0002) sẽ bị phần cứng bxCAN loại bỏ hoàn toàn
 *      ngay tại phần cứng, không tốn bất kỳ chu kỳ CPU nào của vi điều khiển!
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* Lấy thiết bị CAN từ devicetree */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* Cấu hình các ngõ ra LED hiển thị (Y0 - PE5, Y1 - PE6) */
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec y1 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* Message Queue nhận frame cho Bộ lọc B (ID mở rộng) */
CAN_MSGQ_DEFINE(ext_msgq, 8);

/*
 * Cấu trúc sự kiện dùng để truyền dữ liệu từ Callback ngắt (ISR) sang Thread ghi log:
 * Giúp giải phóng nhanh ISR, tuân thủ nguyên tắc "ISR chạy càng nhanh càng tốt".
 */
struct rx_evt {
	uint32_t id;
	uint8_t flags;
};
K_MSGQ_DEFINE(evt_q, sizeof(struct rx_evt), 16, 4);

/* Biến đếm nguyên tử số frame lọt qua từng bộ lọc */
static atomic_t cnt_a, cnt_b;

/*
 * ============================================================================
 * CALLBACK BỘ LỌC A (CHẠY TRONG NGỮ CẢNH NGẮT - ISR):
 * Hàm này được phần cứng gọi trực tiếp khi có frame lọt qua Bộ lọc A.
 * QUY TẮC BẤT DI BẤT DỊCH TRONG ISR:
 * - KHÔNG gọi printf / printk dạng blocking.
 * - KHÔNG gọi k_sleep, k_msleep hay bất kỳ hàm chờ nào.
 * - KHÔNG dùng timeout khác ngoài K_NO_WAIT khi put vào message queue.
 * ============================================================================
 */
static void filter_a_cb(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct rx_evt e = {
		.id = frame->id,
		.flags = frame->flags
	};

	/* Tăng biến đếm an toàn luồng */
	atomic_inc(&cnt_a);

	/* Đưa sự kiện vào queue chuyển sang thread với timeout K_NO_WAIT */
	k_msgq_put(&evt_q, &e, K_NO_WAIT);
}

/*
 * Thread xử lý log cho Bộ lọc A:
 * Lấy sự kiện từ evt_q, in thông tin ra Serial và đảo trạng thái LED Y0.
 */
static void thread_log_a(void *p1, void *p2, void *p3)
{
	struct rx_evt e;

	while (1) {
		k_msgq_get(&evt_q, &e, K_FOREVER);
		printf("    -> [Bo loc A - Callback]: ID=0x%03x %s\n",
		       e.id, (e.flags & CAN_FRAME_RTR) ? "[Khung RTR]" : "[Khung Data]");
		gpio_pin_toggle_dt(&y0);
	}
}
K_THREAD_DEFINE(log_a_tid, 1024, thread_log_a, NULL, NULL, NULL, 5, 0, 0);

/*
 * Thread xử lý cho Bộ lọc B (Message Queue):
 * Đọc trực tiếp struct can_frame từ ext_msgq và đảo trạng thái LED Y1.
 */
static void thread_rx_b(void *p1, void *p2, void *p3)
{
	struct can_frame f;

	while (1) {
		k_msgq_get(&ext_msgq, &f, K_FOREVER);
		atomic_inc(&cnt_b);
		printf("    -> [Bo loc B - MsgQ]:     ID=0x%08x (Extended 29-bit) DATA[0]=0x%02x\n",
		       f.id, f.data[0]);
		gpio_pin_toggle_dt(&y1);
	}
}
K_THREAD_DEFINE(rx_b_tid, 1024, thread_rx_b, NULL, NULL, NULL, 5, 0, 0);

/*
 * Danh sách 9 kịch bản frame thử nghiệm:
 * - 0x100..0x10F: Thuộc dải bộ lọc A (lọt qua A).
 * - 0x110, 0x200: Ngoài dải bộ lọc A và B (BỊ LOẠI BỎ BỞI PHẦN CỨNG).
 * - 0x107 kèm cờ CAN_FRAME_RTR: Khung RTR trong dải bộ lọc A (lọt qua A).
 * - 0x18FF0001 (IDE): Khớp chính xác bộ lọc B (lọt qua B).
 * - 0x18FF0002 (IDE): Sai bit cuối so với bộ lọc B (BỊ LOẠI BỎ BỞI PHẦN CỨNG).
 * - 0x100 (IDE): Cùng giá trị 0x100 nhưng là ID mở rộng (BỊ LOẠI BỎ vì Bộ lọc A chỉ nhận ID chuẩn).
 */
static const struct {
	uint32_t id;
	uint8_t flags;
} tests[] = {
	{0x100, 0},                     /* Lọt qua lọc A */
	{0x105, 0},                     /* Lọt qua lọc A */
	{0x10F, 0},                     /* Lọt qua lọc A */
	{0x110, 0},                     /* Bị loại (ngoài mask 0x7F0) */
	{0x200, 0},                     /* Bị loại */
	{0x107, CAN_FRAME_RTR},         /* Lọt qua lọc A (khung RTR) */
	{0x18FF0001, CAN_FRAME_IDE},    /* Lọt qua lọc B (ID mở rộng đúng) */
	{0x18FF0002, CAN_FRAME_IDE},    /* Bị loại (sai ID mở rộng) */
	{0x100, CAN_FRAME_IDE},         /* Bị loại (Bộ lọc A cờ flags=0 chỉ nhận ID chuẩn) */
};

int main(void)
{
	/*
	 * Cấu hình Bộ lọc A:
	 * - ID = 0x100, MASK = 0x7F0:
	 *   0x7F0 = 111 1111 0000 (nhị phân).
	 *   4 bit cuối là 0 (don't care), 7 bit đầu phải là 001 0000.
	 *   Do đó nhận toàn bộ ID từ 0x100 đến 0x10F.
	 */
	const struct can_filter filter_a = {
		.id = 0x100,
		.mask = 0x7F0,
		.flags = 0,             /* Chỉ nhận Standard ID (11-bit) */
	};

	/*
	 * Cấu hình Bộ lọc B:
	 * - ID = 0x18FF0001, MASK = CAN_EXT_ID_MASK (0x1FFFFFFF):
	 *   So khớp toàn bộ 29 bit.
	 * - flags = CAN_FILTER_IDE: Bắt buộc chỉ nhận Extended ID frame.
	 */
	const struct can_filter filter_b = {
		.id = 0x18FF0001,
		.mask = CAN_EXT_ID_MASK,
		.flags = CAN_FILTER_IDE,
	};

	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y1, GPIO_OUTPUT_INACTIVE);

	printf("\n============================================\n");
	printf("BKIT ARM4 - Lab 3 - Demo 2: CAN Filter Banks\n");
	printf("============================================\n");

	if (!device_is_ready(can_dev)) {
		printf("Loi: Thiet bi CAN chua san sang!\n");
		return 0;
	}

	/* BƯỚC 1: Chọn chế độ Loopback */
	if (can_set_mode(can_dev, CAN_MODE_LOOPBACK) != 0) {
		printf("Loi can_set_mode!\n");
		return 0;
	}

	/* BƯỚC 2: Cài đặt 2 bộ lọc vào phần cứng bxCAN */
	/* Bộ lọc A liên kết với hàm Callback filter_a_cb */
	int fa_id = can_add_rx_filter(can_dev, filter_a_cb, NULL, &filter_a);
	/* Bộ lọc B liên kết với hàng đợi ext_msgq */
	int fb_id = can_add_rx_filter_msgq(can_dev, &ext_msgq, &filter_b);

	printf("Bo loc A dang ky ID #%d (0x100..0x10F qua Callback)\n", fa_id);
	printf("Bo loc B dang ky ID #%d (0x18FF0001 qua MsgQ)\n", fb_id);

	/* BƯỚC 3: Khởi động bộ điều khiển CAN */
	can_start(can_dev);
	printf("CAN Controller da khoi dong thanh cong!\n\n");

	/*
	 * Vòng lặp phát các frame thử nghiệm và thống kê hiệu quả lọc phần cứng
	 */
	for (int round = 1; ; round++) {
		printf("\n----- VONG THU NGHIEM %d -----\n", round);
		for (int i = 0; i < ARRAY_SIZE(tests); i++) {
			struct can_frame f = {
				.id = tests[i].id,
				.flags = tests[i].flags,
				.dlc = (tests[i].flags & CAN_FRAME_RTR) ? 0 : 1,
			};
			f.data[0] = i;

			if (tests[i].flags & CAN_FRAME_IDE) {
				printf("TX Frame: ID=0x%08x [Extended 29-bit]\n", f.id);
			} else {
				printf("TX Frame: ID=0x%03x %s\n",
				       f.id, (f.flags & CAN_FRAME_RTR) ? "[RTR Request]" : "[Data]");
			}

			/* Gửi frame vào bus nội bộ */
			can_send(can_dev, &f, K_MSEC(100), NULL, NULL);
			k_msleep(300);
		}

		/* Thống kê kết quả sau mỗi vòng kiểm tra */
		long total_sent = (long)(round * ARRAY_SIZE(tests));
		long passed_a = atomic_get(&cnt_a);
		long passed_b = atomic_get(&cnt_b);
		long dropped = total_sent - passed_a - passed_b;

		printf(">> THONG KE: Tong gui=%ld | Lot qua Loc A=%ld | Lot qua Loc B=%ld | Bi phan cung loai=%ld\n",
		       total_sent, passed_a, passed_b, dropped);

		k_msleep(5000);
	}

	return 0;
}
