/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * BÀI TẬP 1: TRUYỀN DỮ LIỆU CẢM BIẾN GIẢ LẬP TRÊN CAN LOOPBACK
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Đóng gói (packet encoding) và giải mã (packet decoding) dữ liệu nhiều byte
 *      trong một frame CAN 8 byte theo định dạng chuẩn Big-Endian (byte cao trước).
 *    - Khảo sát cơ chế phát hiện mất frame thông qua bộ đếm số thứ tự tuần tự (seq).
 *    - Đo lường độ trễ truyền nhận (Latency) giữa thời điểm gửi và thời điểm giải mã.
 *    - Thu thập thống kê định kỳ 5 giây về số frame gửi, nhận và tỷ lệ thất thoát.
 *
 * 2. ĐỊNH DẠNG PAYLOAD CAN FRAME (ID chuẩn 0x321, DLC = 8 byte, Big-Endian):
 *    - Byte 0-1: seq (uint16_t) - Số thứ tự frame, tăng 1 sau mỗi lần gửi.
 *    - Byte 2-3: temp_x10 (uint16_t) - Nhiệt độ x10 (250 -> 300, tương ứng 25.0°C -> 30.0°C).
 *    - Byte 4-7: t_ms (uint32_t) - Mốc thời gian gửi (k_uptime_get_32()).
 *
 * 3. HÀNH VI PHẦN CỨNG:
 *    - LED3 (PE4): Đảo trạng thái mỗi khi nhận thành công một frame hợp lệ.
 *    - Ngõ ra Y0 (PE5): Bật sáng nếu phát hiện seq không liên tục (gap/mất frame).
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* Định danh ID chuẩn của gói cảm biến */
#define SENSOR_CAN_ID 0x321

/* Thiết bị CAN và GPIO trên Kit BKIT ARM4 */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios); /* PE4 */
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios); /* PE5 */

/* Hàng đợi chứa frame CAN nhận được */
CAN_MSGQ_DEFINE(sensor_rx_msgq, 16);

/* Biến đếm thống kê */
static atomic_t tx_count = ATOMIC_INIT(0);
static atomic_t rx_count = ATOMIC_INIT(0);
static atomic_t gap_count = ATOMIC_INIT(0);

/*
 * ============================================================================
 * THREAD PHÁT DỮ LIỆU CẢM BIẾN (TX THREAD):
 * - Chu kỳ gửi: 200 ms (tương đương 5 frame/giây).
 * - Đóng gói dữ liệu Big-Endian vào buffer 8 byte của struct can_frame.
 * ============================================================================
 */
static void thread_sensor_tx(void *p1, void *p2, void *p3)
{
	struct can_frame frame = {
		.id = SENSOR_CAN_ID,
		.dlc = 8,
		.flags = 0, /* Standard Data Frame */
	};

	uint16_t seq = 0;
	uint16_t temp_x10 = 250; /* Bắt đầu từ 25.0°C */

	while (1) {
		uint32_t t_ms = k_uptime_get_32();

		/* 
		 * ĐÓNG GÓI DỮ LIỆU THEO CHUẨN BIG-ENDIAN (Most Significant Byte trước):
		 * - Byte 0: Byte cao của seq (seq >> 8)
		 * - Byte 1: Byte thấp của seq (seq & 0xFF)
		 */
		frame.data[0] = (uint8_t)((seq >> 8) & 0xFF);
		frame.data[1] = (uint8_t)(seq & 0xFF);

		/*
		 * - Byte 2: Byte cao của temp_x10
		 * - Byte 3: Byte thấp của temp_x10
		 */
		frame.data[2] = (uint8_t)((temp_x10 >> 8) & 0xFF);
		frame.data[3] = (uint8_t)(temp_x10 & 0xFF);

		/*
		 * - Byte 4-7: Timestamp 32-bit (t_ms)
		 */
		frame.data[4] = (uint8_t)((t_ms >> 24) & 0xFF);
		frame.data[5] = (uint8_t)((t_ms >> 16) & 0xFF);
		frame.data[6] = (uint8_t)((t_ms >> 8) & 0xFF);
		frame.data[7] = (uint8_t)(t_ms & 0xFF);

		/* Gửi frame vào CAN controller (chế độ blocking 50ms) */
		int ret = can_send(can_dev, &frame, K_MSEC(50), NULL, NULL);
		if (ret == 0) {
			atomic_inc(&tx_count);
		} else {
			printf("[%6u ms] [TX LOI] can_send that bai: %d\n", t_ms, ret);
		}

		/* Cập nhật các trường cho lần gửi tiếp theo */
		seq++;
		temp_x10++;
		if (temp_x10 > 300) {
			temp_x10 = 250; /* Quay vòng về 25.0°C */
		}

		k_msleep(200); /* Chu kỳ 200 ms */
	}
}
K_THREAD_DEFINE(sensor_tx_tid, 1024, thread_sensor_tx, NULL, NULL, NULL, 6, 0, 500);

/*
 * ============================================================================
 * THREAD NHẬN VÀ GIẢI MÃ DỮ LIỆU CẢM BIẾN (RX THREAD):
 * - Đọc frame từ sensor_rx_msgq.
 * - Giải mã Big-Endian, đo latency và kiểm tra chuỗi tuần tự của seq.
 * ============================================================================
 */
static void thread_sensor_rx(void *p1, void *p2, void *p3)
{
	struct can_frame frame;
	int32_t expected_seq = -1; /* -1: chưa nhận frame đầu tiên */

	while (1) {
		/* Chờ frame tới */
		k_msgq_get(&sensor_rx_msgq, &frame, K_FOREVER);
		uint32_t now = k_uptime_get_32();
		atomic_inc(&rx_count);

		/* Đảo trạng thái LED3 báo hiệu đã nhận frame */
		gpio_pin_toggle_dt(&led3);

		/*
		 * GIẢI MÃ DỮ LIỆU THEO CHUẨN BIG-ENDIAN:
		 * Đọc byte cao dịch sang trái kết hợp với byte thấp.
		 */
		uint16_t seq = ((uint16_t)frame.data[0] << 8) | frame.data[1];
		uint16_t temp_x10 = ((uint16_t)frame.data[2] << 8) | frame.data[3];
		uint32_t t_ms = ((uint32_t)frame.data[4] << 24) |
		                ((uint32_t)frame.data[5] << 16) |
		                ((uint32_t)frame.data[6] << 8)  |
		                ((uint32_t)frame.data[7]);

		uint32_t latency = (now >= t_ms) ? (now - t_ms) : 0;

		/* In kết quả giải mã nhiệt độ và độ trễ */
		printf("[%6u ms] [RX SENSOR] seq=%5u | Temp=%2u.%1u C | Tre=%u ms\n",
		       now, seq, temp_x10 / 10, temp_x10 % 10, latency);

		/* KIỂM TRA TÍNH LIÊN TỤC CỦA SEQ (PHÁT HIỆN MẤT GÓI) */
		if (expected_seq != -1) {
			if (seq != (uint16_t)expected_seq) {
				atomic_inc(&gap_count);
				/* Bật sáng Y0 cảnh báo đứt gãy tuần tự */
				gpio_pin_set_dt(&y0, 1);
				printf("[%6u ms] >>> CANH BAO: Phat hien gap! Mong doi seq=%u nhung nhan duoc=%u <<<\n",
				       now, expected_seq, seq);
			}
		}
		expected_seq = (seq + 1) & 0xFFFF;
	}
}
K_THREAD_DEFINE(sensor_rx_tid, 1024, thread_sensor_rx, NULL, NULL, NULL, 5, 0, 500);

/*
 * ============================================================================
 * THREAD THỐNG KÊ ĐỊNH KỲ (MỖI 5 GIÂY):
 * In số frame đã gửi, đã nhận và số frame gap.
 * ============================================================================
 */
static void thread_stats(void *p1, void *p2, void *p3)
{
	while (1) {
		k_msleep(5000);
		printf("\n--------------------------------------------------------------\n");
		printf("[%6u ms] [THONG KE 5S] Da gui: %ld | Da nhan: %ld | n_gap: %ld\n",
		       k_uptime_get_32(),
		       atomic_get(&tx_count),
		       atomic_get(&rx_count),
		       atomic_get(&gap_count));
		printf("--------------------------------------------------------------\n\n");
	}
}
K_THREAD_DEFINE(stats_tid, 1024, thread_stats, NULL, NULL, NULL, 7, 0, 500);

int main(void)
{
	/* Bộ lọc: Chỉ chấp nhận ID chuẩn 0x321 (Mask so khớp 11-bit tuyệt đối) */
	const struct can_filter filter = {
		.id = SENSOR_CAN_ID,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};

	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

	printf("\n=======================================================\n");
	printf("BKIT ARM4 - Lab 3 - Bai 1: Sensor Loopback & Packing\n");
	printf("ID: 0x%03x | Chu ky: 200ms | Dinh dang: Big-Endian\n", SENSOR_CAN_ID);
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

	/* 2. Cài đặt bộ lọc phần cứng vào sensor_rx_msgq */
	int filter_id = can_add_rx_filter_msgq(can_dev, &sensor_rx_msgq, &filter);
	if (filter_id < 0) {
		printf("Loi can_add_rx_filter_msgq: %d\n", filter_id);
		return 0;
	}
	printf("Bo loc duoc gan thanh cong (Filter ID = %d)\n", filter_id);

	/* 3. Khởi động CAN controller */
	if (can_start(can_dev) != 0) {
		printf("Loi can_start!\n");
		return 0;
	}
	printf("CAN Controller da san sang chay!\n\n");

	return 0;
}
