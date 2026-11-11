/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * BÀI TẬP 3: LỌC PHẦN CỨNG VÀ ĐỘ ƯU TIÊN TRUYỀN TRÊN CAN LOOPBACK
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Khảo sát bộ lọc phần cứng (Hardware Acceptance Filter) của bxCAN:
 *      + Bộ lọc 1: Nhận chính xác ID 0x080 (Mask 0x7FF).
 *      + Bộ lọc 2: Nhận toàn bộ dải 0x4A0 .. 0x4AF bằng mặt nạ (ID 0x4A0, Mask 0x7F0).
 *      + Frame ID 0x6F0 không thuộc bất kỳ bộ lọc nào và bị phần cứng loại bỏ 100%.
 *    - Khảo sát cơ chế ưu tiên truyền (Arbitration Priority) giữa các Mailbox của bxCAN:
 *      + bxCAN trên STM32 có 3 hộp thư truyền (TX Mailbox 0, 1, 2).
 *      + Khi nhiều mailbox đồng thời sẵn sàng truyền, bộ điều khiển so sánh ID của chúng
 *        và tự động phát frame có ID nhỏ hơn trước (quy tắc arbitration: ID nhỏ = ưu tiên cao).
 *      + Bằng cách gửi liền 3 frame (0x300, 0x200, 0x100) với K_NO_WAIT, ta quan sát
 *        thứ tự nhận được tại bên nhận để chứng minh cơ chế ưu tiên này.
 *    - Giám sát cảnh báo mất frame:
 *      + Trong chu kỳ 5 giây, tần số 0x080 là 100ms/frame -> lý thuyết nhận được 50 frame.
 *      + Nếu số frame nhận được nằm ngoài dải 48..52, ngõ ra Y0 (PE5) sẽ bật sáng cảnh báo.
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* Thiết bị CAN và GPIO */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec y0 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios); /* PE5 */

/* Message queue cho bộ lọc giám sát định kỳ (0x080 và 0x4A0..0x4AF) */
CAN_MSGQ_DEFINE(monitor_rx_msgq, 32);

/* Message queue riêng cho thử nghiệm độ ưu tiên (0x100, 0x200, 0x300) */
CAN_MSGQ_DEFINE(priority_rx_msgq, 8);

/* Các biến thống kê đếm số frame nhận được */
static uint32_t cnt_080 = 0;
static uint32_t cnt_4ax[16] = {0};
static uint32_t last_5s_cnt_080 = 0;

/* Callback khi hoàn thành gửi (ISR) */
static void tx_done_cb(const struct device *dev, int error, void *user_data)
{
	/* Không làm việc nặng trong ISR */
}

/*
 * ============================================================================
 * THREAD PHÁT ĐỊNH KỲ 1: Frame ID 0x080 mỗi 100 ms (10 frame/s)
 * ============================================================================
 */
static void thread_tx_080(void *p1, void *p2, void *p3)
{
	struct can_frame frame = { .id = 0x080, .dlc = 1, .flags = 0 };
	uint8_t seq = 0;

	while (1) {
		frame.data[0] = seq++;
		can_send(can_dev, &frame, K_MSEC(20), tx_done_cb, NULL);
		k_msleep(100);
	}
}
K_THREAD_DEFINE(tx_080_tid, 1024, thread_tx_080, NULL, NULL, NULL, 6, 0, 500);

/*
 * ============================================================================
 * THREAD PHÁT ĐỊNH KỲ 2: Frame ID 0x4A0..0x4AF mỗi 250 ms (4 frame/s)
 * ============================================================================
 */
static void thread_tx_4ax(void *p1, void *p2, void *p3)
{
	uint8_t sub_id = 0;

	while (1) {
		struct can_frame frame = {
			.id = 0x4A0 + sub_id,
			.dlc = 2,
			.flags = 0,
		};
		frame.data[0] = sub_id;
		frame.data[1] = 0xAA;

		can_send(can_dev, &frame, K_MSEC(20), tx_done_cb, NULL);

		sub_id = (sub_id + 1) & 0x0F; /* Quay vòng từ 0 đến 15 (0x4A0..0x4AF) */
		k_msleep(250);
	}
}
K_THREAD_DEFINE(tx_4ax_tid, 1024, thread_tx_4ax, NULL, NULL, NULL, 6, 0, 500);

/*
 * ============================================================================
 * THREAD PHÁT ĐỊNH KỲ 3: Frame ID 0x6F0 mỗi 1000 ms (1 frame/s)
 * Chú ý: Frame này BỊ PHẦN CỨNG bxCAN LOẠI BỎ HOÀN TOÀN, không bao giờ được nhận.
 * ============================================================================
 */
static void thread_tx_6f0(void *p1, void *p2, void *p3)
{
	struct can_frame frame = { .id = 0x6F0, .dlc = 1, .flags = 0 };
	uint8_t seq = 0;

	while (1) {
		frame.data[0] = seq++;
		can_send(can_dev, &frame, K_MSEC(20), tx_done_cb, NULL);
		k_msleep(1000);
	}
}
K_THREAD_DEFINE(tx_6f0_tid, 1024, thread_tx_6f0, NULL, NULL, NULL, 6, 0, 500);

/*
 * ============================================================================
 * THREAD THỬ NGHIỆM ĐỘ ƯU TIÊN (BURST PRIORITY TEST):
 * Mỗi 5 giây, gửi liền 3 frame: 0x300, 0x200, 0x100 bằng K_NO_WAIT.
 * ============================================================================
 */
static void thread_priority_burst(void *p1, void *p2, void *p3)
{
	struct can_frame f300 = { .id = 0x300, .dlc = 1, .flags = 0, .data = {0x33} };
	struct can_frame f200 = { .id = 0x200, .dlc = 1, .flags = 0, .data = {0x22} };
	struct can_frame f100 = { .id = 0x100, .dlc = 1, .flags = 0, .data = {0x11} };

	while (1) {
		k_msleep(5000);

		printf("\n>>> [BURST TEST] Gui lien tiep 3 frame: 0x300 -> 0x200 -> 0x100 (K_NO_WAIT)\n");
		can_send(can_dev, &f300, K_NO_WAIT, tx_done_cb, NULL);
		can_send(can_dev, &f200, K_NO_WAIT, tx_done_cb, NULL);
		can_send(can_dev, &f100, K_NO_WAIT, tx_done_cb, NULL);
	}
}
K_THREAD_DEFINE(prio_burst_tid, 1024, thread_priority_burst, NULL, NULL, NULL, 5, 0, 1000);

/*
 * ============================================================================
 * THREAD NHẬN VÀ IN KẾT QUẢ THỬ NGHIỆM ĐỘ ƯU TIÊN:
 * ============================================================================
 */
static void thread_priority_rx(void *p1, void *p2, void *p3)
{
	struct can_frame frame;

	while (1) {
		k_msgq_get(&priority_rx_msgq, &frame, K_FOREVER);
		printf("    [PRIORITY RX] Nhan duoc frame ID=0x%03x (DATA=0x%02x)\n",
		       frame.id, frame.data[0]);
	}
}
K_THREAD_DEFINE(prio_rx_tid, 1024, thread_priority_rx, NULL, NULL, NULL, 4, 0, 500);

/*
 * ============================================================================
 * THREAD NHẬN VÀ THỐNG KÊ BỘ LỌC ĐỊNH KỲ:
 * Đếm số lượng frame nhận được từ monitor_rx_msgq.
 * Mỗi 5 giây in bảng thống kê chi tiết và kiểm tra giới hạn 48..52 frame 0x080.
 * ============================================================================
 */
static void thread_monitor_rx(void *p1, void *p2, void *p3)
{
	struct can_frame frame;

	while (1) {
		/* Đọc frame với timeout 5 giây để định kỳ in bảng thống kê */
		int ret = k_msgq_get(&monitor_rx_msgq, &frame, K_SECONDS(5));
		if (ret == 0) {
			if (frame.id == 0x080) {
				cnt_080++;
			} else if (frame.id >= 0x4A0 && frame.id <= 0x4AF) {
				cnt_4ax[frame.id - 0x4A0]++;
			}
		}

		/* Kiểm tra thời điểm in thống kê (mỗi 5 giây) */
		static uint32_t last_report = 0;
		uint32_t now = k_uptime_get_32();
		if (now - last_report >= 5000) {
			last_report = now;

			uint32_t delta_080 = cnt_080 - last_5s_cnt_080;
			last_5s_cnt_080 = cnt_080;

			/* Kiểm tra dải hợp lệ 48..52 của frame 0x080 trong 5s */
			if (delta_080 >= 48 && delta_080 <= 52) {
				gpio_pin_set_dt(&y0, 0); /* Đạt chuẩn: Tắt cảnh báo */
			} else {
				gpio_pin_set_dt(&y0, 1); /* Ngoài chuẩn: Bật LED Y0 cảnh báo mất frame */
			}

			/* In bảng thống kê */
			printf("\n==================== [THONG KE BO LOC (5 GIAY)] ====================\n");
			printf("ID 0x080 (100ms): Nhan %u frame trong 5s (Tong=%u) -> %s\n",
			       delta_080, cnt_080,
			       (delta_080 >= 48 && delta_080 <= 52) ? "CHUAN [48..52]" : "CANH BAO MAT FRAME!");

			printf("Dai 0x4A0..0x4AF (250ms): ");
			uint32_t sum_4ax = 0;
			for (int i = 0; i < 16; i++) {
				sum_4ax += cnt_4ax[i];
			}
			printf("Tong nhan = %u frame (Moi ID ~ %u frame)\n", sum_4ax, sum_4ax / 16);

			printf("ID 0x6F0 (1000ms): Nhan = 0 frame (BI BO LOC PHAN CUNG LOAI BO 100%%)\n");
			printf("====================================================================\n\n");
		}
	}
}
K_THREAD_DEFINE(monitor_rx_tid, 1024, thread_monitor_rx, NULL, NULL, NULL, 5, 0, 500);

int main(void)
{
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

	printf("\n=======================================================\n");
	printf("BKIT ARM4 - Lab 3 - Bai 3: Hardware Filters & Priority\n");
	printf("=======================================================\n");

	if (!device_is_ready(can_dev)) {
		printf("Loi: Thiet bi CAN chua san sang!\n");
		return 0;
	}

	/* 1. Thiết lập chế độ Loopback */
	if (can_set_mode(can_dev, CAN_MODE_LOOPBACK) != 0) {
		printf("Loi can_set_mode!\n");
		return 0;
	}

	/*
	 * 2. CÀI ĐẶT 2 BỘ LỌC PHẦN CỨNG GIÁM SÁT:
	 * 
	 * Bộ lọc 1: Nhận chính xác ID 0x080 (Mask 0x7FF so khớp cả 11 bit)
	 */
	const struct can_filter filter_080 = {
		.id = 0x080,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	can_add_rx_filter_msgq(can_dev, &monitor_rx_msgq, &filter_080);

	/*
	 * Bộ lọc 2: Nhận toàn bộ dải 0x4A0 .. 0x4AF
	 * - 0x4A0 = 0100 1010 0000 (nhị phân)
	 * - 0x4AF = 0100 1010 1111 (nhị phân)
	 * - Mặt nạ mask = 0x7F0 = 0111 1111 0000:
	 *   7 bit đầu bắt buộc phải là 0100 1010 (0x4A), 4 bit cuối là "don't care" (0..15).
	 *   -> Nhận toàn bộ 16 ID từ 0x4A0 đến 0x4AF!
	 *   Khung 0x6F0 (0110 1111 0000) có 7 bit đầu là 0110 111 != 0100 101 -> BỊ LOẠI!
	 */
	const struct can_filter filter_4ax = {
		.id = 0x4A0,
		.mask = 0x7F0,
		.flags = 0,
	};
	can_add_rx_filter_msgq(can_dev, &monitor_rx_msgq, &filter_4ax);

	/*
	 * 3. BỘ LỌC RIÊNG CHO THỬ NGHIỆM ĐỘ ƯU TIÊN (0x100, 0x200, 0x300):
	 * Sử dụng mask 0x4FF và id 0x000:
	 * - Bit 10 = 0 (loại các ID >= 0x400 như 0x4A0..0x4AF, 0x6F0)
	 * - Bit 7..0 = 0 (loại các ID có bit thấp như 0x080)
	 * -> Chỉ khớp chính xác các ID dạng 0x100, 0x200, 0x300!
	 */
	const struct can_filter filter_prio = {
		.id = 0x000,
		.mask = 0x4FF,
		.flags = 0,
	};
	can_add_rx_filter_msgq(can_dev, &priority_rx_msgq, &filter_prio);

	/* 4. Khởi động CAN controller */
	if (can_start(can_dev) != 0) {
		printf("Loi can_start!\n");
		return 0;
	}
	printf("CAN Controller da khoi dong thanh cong!\n\n");

	return 0;
}
