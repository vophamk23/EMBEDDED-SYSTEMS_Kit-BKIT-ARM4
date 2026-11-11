/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * BÀI TẬP 2: ĐIỀU KHIỂN ĐÈN BẰNG LỆNH CAN VÀ FRAME RTR (LOOPBACK)
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Thiết kế giao thức lệnh/phản hồi (Command/Response protocol) trên mạng CAN.
 *    - Sử dụng khung yêu cầu truyền dữ liệu từ xa RTR (Remote Transmission Request).
 *    - Mô phỏng 2 thực thể trên cùng một kit vi điều khiển (chế độ Loopback):
 *      + thread_controller: Gửi lệnh điều khiển bật/tắt đèn và gửi yêu cầu RTR hỏi trạng thái.
 *      + thread_actuator: Lắng nghe lệnh thực thi trên phần cứng (LED3, Y0, Y1),
 *        trả về phản hồi xác nhận (ACK/NAK) hoặc dữ liệu bitmap khi nhận RTR.
 *
 * 2. ĐỊNH NGHĨA GIAO THỨC TRUYỀN THÔNG:
 *    + ID 0x200 (Controller -> Actuator):
 *        DLC = 2. data[0] = Mã đèn (0: LED3, 1: Y0, 2: Y1). data[1] = Trạng thái (0: Tắt, 1: Bật).
 *    + ID 0x201 (Actuator -> Controller):
 *        DLC = 3. data[0] = Mã đèn, data[1] = Trạng thái mới, data[2] = Kết quả (0: Thành công OK, 1: Lỗi đèn không hợp lệ).
 *    + ID 0x202 RTR (Controller -> Actuator):
 *        DLC = 0, cờ CAN_FRAME_RTR: "Yêu cầu Actuator báo cáo trạng thái toàn bộ đèn".
 *    + ID 0x202 DATA (Actuator -> Controller):
 *        DLC = 1. data[0] = Bitmap trạng thái (Bit 0: LED3, Bit 1: Y0, Bit 2: Y1).
 *
 * 3. TỔ CHỨC BỘ LỌC PHẦN CỨNG VÀ XỬ LÝ LOOPBACK:
 *    - Actuator có 2 bộ lọc: ID 0x200 (lệnh) và ID 0x202 (truy vấn trạng thái).
 *      Actuator phân biệt lệnh và RTR bằng kiểm tra cờ (frame.flags & CAN_FRAME_RTR).
 *    - Controller có 2 bộ lọc riêng cho phản hồi: ID 0x201 và ID 0x202.
 *      Ở chế độ Loopback, Controller cũng tự nhận lại frame RTR 0x202 do chính nó gửi;
 *      Controller phân biệt bằng cách kiểm tra: nếu (frame.flags & CAN_FRAME_RTR) thì bỏ qua,
 *      chỉ xử lý khi frame là Data frame (!(frame.flags & CAN_FRAME_RTR)).
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* Các ID định nghĩa trong giao thức */
#define CAN_ID_CMD         0x200  /* Controller -> Actuator: Lenh dieu khien */
#define CAN_ID_RESP        0x201  /* Actuator -> Controller: Phan hoi lenh */
#define CAN_ID_RTR_STATUS  0x202  /* RTR / Data Status: Truy van & tra loi bitmap */

/* Thiết bị CAN */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* 3 ngõ ra điều khiển đèn trên kit BKIT ARM4 */
static const struct gpio_dt_spec leds[3] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios), /* Đèn 0: LED3 (PE4) */
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios), /* Đèn 1: Y0   (PE5) */
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios), /* Đèn 2: Y1   (PE6) */
};

/* Hàng đợi tin nhắn riêng cho Actuator và Controller */
CAN_MSGQ_DEFINE(actuator_msgq, 8);
CAN_MSGQ_DEFINE(controller_msgq, 8);

/* Trạng thái hiện tại của 3 đèn (0: Tắt, 1: Bật) */
static uint8_t led_states[3] = {0, 0, 0};

/*
 * ============================================================================
 * THREAD ACTUATOR (BỘ CHẤP HÀNH):
 * - Lắng nghe các frame từ actuator_msgq (ID 0x200 hoặc 0x202 RTR).
 * - Xử lý bật/tắt đèn và gửi frame phản hồi tương ứng.
 * ============================================================================
 */
static void thread_actuator(void *p1, void *p2, void *p3)
{
	struct can_frame rx_frame;

	while (1) {
		k_msgq_get(&actuator_msgq, &rx_frame, K_FOREVER);

		/* TRƯỜNG HỢP 1: Nhận lệnh điều khiển ID 0x200 (Data Frame) */
		if (rx_frame.id == CAN_ID_CMD && !(rx_frame.flags & CAN_FRAME_RTR)) {
			uint8_t led_idx = rx_frame.data[0];
			uint8_t state   = rx_frame.data[1];

			struct can_frame resp_frame = {
				.id = CAN_ID_RESP,
				.dlc = 3,
				.flags = 0,
			};
			resp_frame.data[0] = led_idx;
			resp_frame.data[1] = state;

			/* Kiểm tra tính hợp lệ của mã đèn (0: LED3, 1: Y0, 2: Y1) */
			if (led_idx < 3) {
				led_states[led_idx] = state ? 1 : 0;
				gpio_pin_set_dt(&leds[led_idx], led_states[led_idx]);
				resp_frame.data[2] = 0; /* 0 = Thành công OK */
				printf("[%6u ms] [ACTUATOR] Thuc thi lenh: Den %u -> %s (OK)\n",
				       k_uptime_get_32(), led_idx, state ? "BAT" : "TAT");
			} else {
				resp_frame.data[2] = 1; /* 1 = Lỗi mã đèn không tồn tại */
				printf("[%6u ms] [ACTUATOR] Tu choi lenh: Den %u KHONG HOP LE (ERROR)\n",
				       k_uptime_get_32(), led_idx);
			}

			/* Gửi phản hồi 0x201 về Controller */
			can_send(can_dev, &resp_frame, K_MSEC(50), NULL, NULL);
		}
		/* TRƯỜNG HỢP 2: Nhận yêu cầu Remote Frame RTR ID 0x202 */
		else if (rx_frame.id == CAN_ID_RTR_STATUS && (rx_frame.flags & CAN_FRAME_RTR)) {
			/* Đóng gói bitmap trạng thái: bit0=LED3, bit1=Y0, bit2=Y1 */
			uint8_t bitmap = (led_states[0] & 1) |
			                 ((led_states[1] & 1) << 1) |
			                 ((led_states[2] & 1) << 2);

			struct can_frame data_frame = {
				.id = CAN_ID_RTR_STATUS,
				.dlc = 1,
				.flags = 0, /* Frame dữ liệu trả lời cho RTR */
			};
			data_frame.data[0] = bitmap;

			printf("[%6u ms] [ACTUATOR] Nhan RTR 0x202 -> Tra loi Bitmap: 0x%02x (LED3=%u, Y0=%u, Y1=%u)\n",
			       k_uptime_get_32(), bitmap, led_states[0], led_states[1], led_states[2]);

			/* Phát frame dữ liệu trả lời lên bus */
			can_send(can_dev, &data_frame, K_MSEC(50), NULL, NULL);
		}
	}
}
K_THREAD_DEFINE(actuator_tid, 1024, thread_actuator, NULL, NULL, NULL, 5, 0, 500);

/*
 * ============================================================================
 * THREAD CONTROLLER (BỘ ĐIỀU KHIỂN):
 * - Kịch bản lặp:
 *   1. Bật lần lượt LED3, Y0, Y1 (cách nhau 1 giây).
 *   2. Gửi RTR 0x202 hỏi trạng thái tất cả đèn.
 *   3. Tắt lần lượt LED3, Y0, Y1 (cách nhau 1 giây).
 *   4. Gửi một lệnh cố ý sai (đèn số 5) để kiểm tra phản hồi báo lỗi.
 *   5. Lắng nghe và in ra toàn bộ phản hồi từ Actuator.
 * ============================================================================
 */
static void send_cmd(uint8_t led_idx, uint8_t state)
{
	struct can_frame cmd = {
		.id = CAN_ID_CMD,
		.dlc = 2,
		.flags = 0,
	};
	cmd.data[0] = led_idx;
	cmd.data[1] = state;

	printf("[%6u ms] [CONTROLLER] Gui lenh 0x200: Den %u -> %s\n",
	       k_uptime_get_32(), led_idx, state ? "BAT" : "TAT");
	can_send(can_dev, &cmd, K_MSEC(50), NULL, NULL);
}

static void send_rtr_status_request(void)
{
	struct can_frame rtr = {
		.id = CAN_ID_RTR_STATUS,
		.dlc = 0,
		.flags = CAN_FRAME_RTR, /* Đặt cờ RTR */
	};

	printf("[%6u ms] [CONTROLLER] Gui khung yeu cau RTR 0x202 (Hoi trang thai bitmap)...\n",
	       k_uptime_get_32());
	can_send(can_dev, &rtr, K_MSEC(50), NULL, NULL);
}

static void thread_controller(void *p1, void *p2, void *p3)
{
	while (1) {
		printf("\n================ [BAT DAU CHU TRINH DIEU KHIEN] ================\n");

		/* Bước 1: Bật lần lượt 3 đèn cách nhau 1s */
		send_cmd(0, 1); /* Bật LED3 */
		k_msleep(1000);

		send_cmd(1, 1); /* Bật Y0 */
		k_msleep(1000);

		send_cmd(2, 1); /* Bật Y1 */
		k_msleep(1000);

		/* Bước 2: Gửi khung RTR 0x202 truy vấn trạng thái */
		send_rtr_status_request();
		k_msleep(1000);

		/* Bước 3: Tắt lần lượt 3 đèn cách nhau 1s */
		send_cmd(0, 0); /* Tắt LED3 */
		k_msleep(1000);

		send_cmd(1, 0); /* Tắt Y0 */
		k_msleep(1000);

		send_cmd(2, 0); /* Tắt Y1 */
		k_msleep(1000);

		/* Bước 4: Gửi lệnh cố ý sai (đèn số 5) kiểm tra lỗi */
		printf("[%6u ms] [CONTROLLER] Gui thu nghiem lenh SAI (Den 5) de test loi...\n", k_uptime_get_32());
		send_cmd(5, 1);
		k_msleep(2000);
	}
}
K_THREAD_DEFINE(ctrl_tx_tid, 1024, thread_controller, NULL, NULL, NULL, 6, 0, 1000);

/*
 * THREAD LẮNG NGHE PHẢN HỒI DÀNH CHO CONTROLLER:
 * Nhận frame từ controller_msgq (bao gồm 0x201 phản hồi lệnh và 0x202 dữ liệu bitmap).
 */
static void thread_controller_rx(void *p1, void *p2, void *p3)
{
	struct can_frame resp;

	while (1) {
		k_msgq_get(&controller_msgq, &resp, K_FOREVER);

		/* Nếu nhận được chính frame RTR do Controller gửi trong Loopback -> Bỏ qua */
		if (resp.flags & CAN_FRAME_RTR) {
			continue;
		}

		if (resp.id == CAN_ID_RESP) {
			uint8_t led_idx = resp.data[0];
			uint8_t state   = resp.data[1];
			uint8_t status  = resp.data[2];

			printf("[%6u ms] [CONTROLLER NHAN RESP 0x201] Den: %u, Trang thai: %s, Ket qua: %s\n",
			       k_uptime_get_32(), led_idx, state ? "BAT" : "TAT",
			       status == 0 ? "THANH CONG (OK)" : "LOI KHONG HOP LE (ERROR)");
		} else if (resp.id == CAN_ID_RTR_STATUS) {
			uint8_t bitmap = resp.data[0];
			printf("[%6u ms] [CONTROLLER NHAN DATA 0x202] Bitmap den: 0x%02x [LED3: %s | Y0: %s | Y1: %s]\n",
			       k_uptime_get_32(), bitmap,
			       (bitmap & 0x01) ? "ON" : "OFF",
			       (bitmap & 0x02) ? "ON" : "OFF",
			       (bitmap & 0x04) ? "ON" : "OFF");
		}
	}
}
K_THREAD_DEFINE(ctrl_rx_tid, 1024, thread_controller_rx, NULL, NULL, NULL, 5, 0, 500);

int main(void)
{
	/* Khởi tạo GPIO */
	for (int i = 0; i < 3; i++) {
		gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
	}

	printf("\n=======================================================\n");
	printf("BKIT ARM4 - Lab 3 - Bai 2: CAN Command & RTR (Loopback)\n");
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
	 * 2. CÀI ĐẶT CÁC BỘ LỌC PHẦN CỨNG:
	 * 
	 * BỘ LỌC CHO ACTUATOR:
	 * - Lọc 1: Nhận lệnh 0x200
	 * - Lọc 2: Nhận yêu cầu RTR 0x202
	 */
	const struct can_filter filter_actuator_cmd = {
		.id = CAN_ID_CMD,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	const struct can_filter filter_actuator_rtr = {
		.id = CAN_ID_RTR_STATUS,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	can_add_rx_filter_msgq(can_dev, &actuator_msgq, &filter_actuator_cmd);
	can_add_rx_filter_msgq(can_dev, &actuator_msgq, &filter_actuator_rtr);

	/*
	 * BỘ LỌC CHO CONTROLLER:
	 * - Lọc 1: Nhận phản hồi lệnh 0x201
	 * - Lọc 2: Nhận dữ liệu bitmap 0x202
	 */
	const struct can_filter filter_ctrl_resp = {
		.id = CAN_ID_RESP,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	const struct can_filter filter_ctrl_data = {
		.id = CAN_ID_RTR_STATUS,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	can_add_rx_filter_msgq(can_dev, &controller_msgq, &filter_ctrl_resp);
	can_add_rx_filter_msgq(can_dev, &controller_msgq, &filter_ctrl_data);

	/* 3. Khởi động CAN controller */
	if (can_start(can_dev) != 0) {
		printf("Loi can_start!\n");
		return 0;
	}
	printf("CAN Controller da khoi dong thanh cong!\n\n");

	return 0;
}
