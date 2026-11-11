/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * DEMO 1: GỬI VÀ NHẬN CAN Ở CHẾ ĐỘ LOOPBACK (VÒNG LẶP NỘI TẠI)
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI DEMO:
 *    - Hiểu cơ chế hoạt động của bộ điều khiển CAN (bxCAN trên STM32F407).
 *    - Làm quen với chế độ Loopback: Bộ điều khiển tự nối chân TX vào RX nội tại.
 *      -> Giúp kiểm tra chức năng truyền/nhận và logic phần mềm chỉ với 1 kit duy nhất,
 *         không cần nối dây ra ngoài, không cần transceiver bên ngoài và không cần node khác gửi ACK.
 *    - Nắm vững chu trình khởi tạo chuẩn của Zephyr CAN API:
 *        can_set_mode() -> can_add_rx_filter_msgq() -> can_start()
 *    - Sử dụng Message Queue (CAN_MSGQ_DEFINE) để nhận frame bất đồng bộ an toàn giữa ISR và Thread.
 *
 * 2. CẤU HÌNH PHẦN CỨNG (Devicetree & Overlay):
 *    - Controller: CAN1 (chân PB8 - RX, PB9 - TX).
 *    - Bitrate cấu hình: 500 kbit/s.
 *    - LED3: PE4 (nhấp nháy mỗi khi nhận thành công một frame).
 *
 * 3. CÁC HÀM ZEPHYR CAN QUAN TRỌNG TRONG BÀI:
 *    - DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)): Lấy con trỏ thiết bị CAN từ devicetree.
 *    - can_set_mode(dev, CAN_MODE_LOOPBACK): Đặt chế độ Loopback (phải gọi trước can_start).
 *    - can_add_rx_filter_msgq(dev, &msgq, &filter): Đăng ký bộ lọc phần cứng; frame thỏa mãn
 *      sẽ được phần cứng bxCAN tự động đẩy vào message queue của hệ điều hành.
 *    - can_start(dev): Kích hoạt bộ điều khiển CAN tham gia vào mạng.
 *    - can_send(dev, &frame, timeout, cb, user_data): Đưa frame vào hộp thư TX (Mailbox).
 *    - can_dlc_to_bytes(dlc): Chuyển đổi mã DLC (0-8) sang số byte dữ liệu thực tế.
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* ID chuẩn 11-bit dùng cho truyền và nhận trong bài demo */
#define TX_ID 0x123

/* Lấy thiết bị CAN được định nghĩa trong app.overlay thông qua zephyr,canbus */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* Cấu hình LED3 (LED Debug trên kit BKIT ARM4, chân PE4) */
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/*
 * CAN_MSGQ_DEFINE(rx_msgq, 8):
 * Macro của Zephyr tạo một Message Queue kích thước 8 phần tử,
 * mỗi phần tử có kiểu dữ liệu là struct can_frame (kích thước ~16 byte).
 * Bộ lọc CAN sẽ tự động sao chép frame nhận được vào hàng đợi này từ ngữ cảnh ngắt (ISR).
 */
CAN_MSGQ_DEFINE(rx_msgq, 8);

/**
 * @brief Hàm in nội dung frame CAN ra console serial (USART1)
 * @param tag Nhãn phân biệt (TX: gửi, RX: nhận)
 * @param f Con trỏ tới struct can_frame
 */
static void print_frame(const char *tag, const struct can_frame *f)
{
	printf("[%6u ms] %s ID=0x%03x DLC=%u DATA=", k_uptime_get_32(), tag, f->id, f->dlc);
	for (int i = 0; i < can_dlc_to_bytes(f->dlc); i++) {
		printf("%02x ", f->data[i]);
	}
	printf("\n");
}

/*
 * ============================================================================
 * THREAD NHẬN (RX THREAD):
 * - Luôn ở trạng thái BLOCKED trên k_msgq_get(&rx_msgq, ...) để không chiếm CPU.
 * - Khi có frame CAN tới, bxCAN kích hoạt ngắt, driver Zephyr đẩy frame vào msgq
 *   và đánh thức thread này dậy.
 * - Thread in frame ra màn hình và đảo trạng thái LED3 để báo hiệu.
 * ============================================================================
 */
static void thread_rx(void *p1, void *p2, void *p3)
{
	struct can_frame frame;

	while (1) {
		/* Chờ vô hạn (K_FOREVER) cho đến khi có frame trong message queue */
		k_msgq_get(&rx_msgq, &frame, K_FOREVER);

		/* In thông tin frame nhận được */
		print_frame("RX", &frame);

		/* Đảo trạng thái LED3 (PE4) */
		gpio_pin_toggle_dt(&led3);
	}
}

/* Định nghĩa thread nhận: Stack 1024 byte, độ ưu tiên 5, chạy ngay sau khi boot */
K_THREAD_DEFINE(rx_tid, 1024, thread_rx, NULL, NULL, NULL, 5, 0, 0);

/*
 * ============================================================================
 * MAIN THREAD (TX):
 * Khởi tạo phần cứng CAN, cấu hình bộ lọc, và định kỳ 1 giây gửi một frame.
 * ============================================================================
 */
int main(void)
{
	/*
	 * Cấu hình Bộ lọc nhận CAN (Hardware Filter):
	 * - .id = TX_ID (0x123): ID mong muốn nhận.
	 * - .mask = CAN_STD_ID_MASK (0x7FF): So khớp tuyệt đối toàn bộ 11 bit ID.
	 *   Quy tắc lọc: (ID_nhan & mask) == (filter.id & mask).
	 *   Với mask 0x7FF, chỉ duy nhất frame có ID = 0x123 mới được lọt qua.
	 * - .flags = 0: ID chuẩn 11-bit (CAN 2.0A).
	 */
	const struct can_filter filter = {
		.id = TX_ID,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};

	/* Khởi tạo cấu trúc frame CAN để gửi */
	struct can_frame frame = {
		.id = TX_ID,
		.dlc = 8,       /* Chiều dài dữ liệu: 8 byte */
		.flags = 0,     /* Frame dữ liệu chuẩn (Data Frame, 11-bit ID) */
	};
	uint8_t counter = 0;
	int ret;

	/* Khởi tạo GPIO cho LED3 */
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

	printf("\n========================================\n");
	printf("BKIT ARM4 - Lab 3 - Demo 1: CAN Loopback\n");
	printf("========================================\n");

	/* Kiểm tra thiết bị CAN đã sẵn sàng theo devicetree chưa */
	if (!device_is_ready(can_dev)) {
		printf("Loi: Thiet bi CAN chua san sang!\n");
		return 0;
	}

	/*
	 * BƯỚC 1: Chọn chế độ hoạt động (BẮT BUỘC thực hiện khi CAN controller đang STOPPED).
	 * CAN_MODE_LOOPBACK: Tín hiệu TX tự động phản hồi lại RX bên trong chip STM32.
	 */
	ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	if (ret != 0) {
		printf("Loi can_set_mode: %d\n", ret);
		return 0;
	}

	/*
	 * BƯỚC 2: Cài đặt bộ lọc phần cứng và liên kết với Message Queue.
	 * Hàm can_add_rx_filter_msgq nạp cấu hình bộ lọc vào các thanh ghi filter bank
	 * của bxCAN. Nếu thành công, hàm trả về filter_id (số không âm).
	 */
	ret = can_add_rx_filter_msgq(can_dev, &rx_msgq, &filter);
	if (ret < 0) {
		printf("Loi can_add_rx_filter_msgq: %d\n", ret);
		return 0;
	}
	printf("Da cai dat Bo loc #%d: ID=0x%03x MASK=0x%03x\n", ret, filter.id, filter.mask);

	/*
	 * BƯỚC 3: Khởi động bộ điều khiển CAN (Chuyển từ STOPPED sang NORMAL/LOOPBACK).
	 * Sau lệnh này, bxCAN bắt đầu tham gia đồng bộ hóa đường truyền.
	 */
	ret = can_start(can_dev);
	if (ret != 0) {
		printf("Loi can_start: %d\n", ret);
		return 0;
	}
	printf("CAN Controller da khoi dong thanh cong!\n\n");

	/*
	 * VÒNG LẶP TRUYỀN DỮ LIỆU:
	 * Mỗi giây cập nhật dữ liệu và gửi 1 frame CAN vào bus nội tại.
	 */
	while (1) {
		/* Điền dữ liệu vào payload 8 byte */
		frame.data[0] = counter++;
		frame.data[1] = 0xCA;
		frame.data[2] = 0xFE;
		for (int i = 3; i < 8; i++) {
			frame.data[i] = i;
		}

		/*
		 * can_send():
		 * - can_dev: Thiết bị CAN.
		 * - &frame: Con trỏ tới frame cần truyền.
		 * - timeout: K_MSEC(100) - Chờ tối đa 100ms nếu cả 3 hộp thư TX Mailbox đang bận.
		 * - callback = NULL: Chế độ truyền đồng bộ (chặn cho tới khi frame gửi xong).
		 * - user_data = NULL.
		 */
		ret = can_send(can_dev, &frame, K_MSEC(100), NULL, NULL);
		if (ret != 0) {
			printf("Loi can_send: %d\n", ret);
		} else {
			print_frame("TX", &frame);
		}

		/* Nghỉ 1000ms trước khi gửi frame tiếp theo */
		k_msleep(1000);
	}

	return 0;
}
