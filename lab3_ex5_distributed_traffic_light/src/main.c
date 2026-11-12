/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * BÀI TẬP 5: HỆ THỐNG ĐÈN GIAO THÔNG PHÂN TÁN QUA CAN (BÀI TỔNG HỢP)
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Xây dựng hệ thống nhúng phân tán 2 node hoàn chỉnh trên mạng CAN:
 *      + Kit A (Controller - Bộ điều khiển): Chạy máy trạng thái (FSM) đèn giao thông
 *        (ĐỎ 7s -> XANH 5s -> VÀNG 2s; XANH tối thiểu 2s, rút ngắn khi có người xin qua đường).
 *        Kit A không tự điều khiển đèn trên kit mình mà broadcast lệnh trạng thái qua CAN.
 *      + Kit B (Display - Bộ hiển thị): Nhận lệnh CAN và trực tiếp điều khiển các ngõ ra
 *        công suất Y0 (Đỏ) và Y1 (Xanh), cả hai cùng bật là Vàng.
 *    - Cơ chế truyền phát (Broadcast) định kỳ và theo sự kiện:
 *      + Mỗi 500 ms VÀ mỗi khi đổi pha, Kit A phát frame 0x100: [pha, số_giây_còn_lại, seq].
 *    - Tương tác hai chiều (Yêu cầu qua đường từ xa):
 *      + Kit B định kỳ mô phỏng nút bấm người đi bộ (sau 10s, 14s, 8s, 17s...) gửi frame 0x110.
 *      + Kit A tiếp nhận yêu cầu, quyết định chấp nhận (nếu đang Xanh) hoặc bỏ qua (Đỏ/Vàng),
 *        sau đó phản hồi frame 0x111 xác nhận.
 *    - Cơ chế an toàn khi mất liên lạc (Fail-Safe Watchdog):
 *      + Kit B giám sát tín hiệu từ Kit A: Nếu quá 2 giây không nhận được frame 0x100,
 *        Kit B tự động chuyển sang chế độ VÀNG NHẤP NHÁY (Y0 + Y1 nháy 500ms ON / 500ms OFF)
 *        và in thông báo [FAIL-SAFE].
 *      + Hệ thống tự động phục hồi ngay khi kết nối CAN hoạt động trở lại.
 *    - Heartbeat & Diagnostics:
 *      + Cả 2 kit nhấp nháy LED3 (PE4) chu kỳ 1s làm heartbeat.
 *      + Đăng ký callback giám sát trạng thái vật lý bus CAN.
 *    - Hỗ trợ DUAL_ROLE_LOOPBACK:
 *      + Cho phép chạy đồng thời cả vai trò Kit A và Kit B trên 1 kit duy nhất ở chế độ Loopback
 *        để chấm điểm độc lập khi thiếu thiết bị.
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* CẤU HÌNH VAI TRÒ */
#define ROLE_KIT_A 1  /* Kit A: Bộ điều khiển trung tâm (Controller) */
#define ROLE_KIT_B 2  /* Kit B: Bộ hiển thị ngoại vi & Nút bấm (Display) */

/* 
 * CHỌN VAI TRÒ CHO BOARD HIỆN TẠI:
 * Đặt NODE_ROLE là ROLE_KIT_A khi nạp Kit A, ROLE_KIT_B khi nạp Kit B.
 * Nếu DUAL_ROLE_LOOPBACK = 1: Chương trình chạy cả 2 vai trò trên 1 Kit (chế độ Loopback).
 */
#define DUAL_ROLE_LOOPBACK 0
#define NODE_ROLE          ROLE_KIT_A

/* Các ID định nghĩa trong giao thức CAN */
#define CAN_ID_TRAFFIC_STATE   0x100  /* Kit A -> Kit B: Broadcast trạng thái đèn */
#define CAN_ID_PED_REQ         0x110  /* Kit B -> Kit A: Yêu cầu người đi bộ xin qua đường */
#define CAN_ID_PED_RESP        0x111  /* Kit A -> Kit B: Phản hồi chấp nhận/bỏ qua */

/* Các pha đèn */
enum traffic_phase {
	PHASE_RED = 0,
	PHASE_GREEN = 1,
	PHASE_YELLOW = 2
};

/* Thiết bị CAN và GPIO */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios); /* PE4: Heartbeat */
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios); /* PE5: Đèn Đỏ */
static const struct gpio_dt_spec y1   = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios); /* PE6: Đèn Xanh */

/* Message queue nhận frame CAN */
CAN_MSGQ_DEFINE(traffic_rx_msgq, 16);

/* Biến đồng bộ và truyền thông nội bộ */
static atomic_t ped_request_flag = ATOMIC_INIT(0);
static atomic_t ped_req_seq_current = ATOMIC_INIT(0);

/* Callback khi gửi hoàn tất */
static void tx_done_cb(const struct device *dev, int error, void *user_data)
{
	/* Không làm việc nặng trong ISR */
}

/*
 * ============================================================================
 * THREAD HEARTBEAT (CHẠY TRÊN CẢ HAI KIT):
 * Đảo trạng thái LED3 (PE4) mỗi giây 1 lần để báo hiệu vi điều khiển còn sống.
 * ============================================================================
 */
static void thread_heartbeat(void *p1, void *p2, void *p3)
{
	while (1) {
		gpio_pin_toggle_dt(&led3);
		k_msleep(1000);
	}
}
K_THREAD_DEFINE(hb_tid, 512, thread_heartbeat, NULL, NULL, NULL, 7, 0, 0);

/*
 * ============================================================================
 * PHẦN DÀNH CHO KIT A: BỘ ĐIỀU KHIỂN TRUNG TÂM (CONTROLLER FSM)
 * ============================================================================
 */
#if (NODE_ROLE == ROLE_KIT_A) || (DUAL_ROLE_LOOPBACK == 1)

/*
 * Hàm phát frame trạng thái 0x100 qua CAN
 */
static void send_traffic_state(enum traffic_phase phase, uint8_t remaining_sec)
{
	static uint8_t seq = 0;
	struct can_frame f = {
		.id = CAN_ID_TRAFFIC_STATE,
		.dlc = 3,
		.flags = 0,
	};
	f.data[0] = (uint8_t)phase;
	f.data[1] = remaining_sec;
	f.data[2] = seq++;

	can_send(can_dev, &f, K_MSEC(100), tx_done_cb, NULL);
}

/*
 * Thread FSM quản lý chu kỳ đèn giao thông của Kit A
 */
static void thread_kit_a_fsm(void *p1, void *p2, void *p3)
{
	enum traffic_phase current_phase = PHASE_RED;
	int phase_duration = 7; /* Đỏ 7s */
	int remaining_sec = phase_duration;
	int elapsed_in_phase = 0;

	printf("[%6u ms] [KIT A] Khoi dong FSM Den giao thong: DO 7s -> XANH 5s -> VANG 2s\n",
	       k_uptime_get_32());

	while (1) {
		/* Gửi trạng thái định kỳ và thông báo */
		send_traffic_state(current_phase, (uint8_t)remaining_sec);

		/* Mỗi giây kiểm tra 2 lần (500ms một lần) */
		for (int sub = 0; sub < 2; sub++) {
			k_msleep(500);

			/* Gửi lại frame 0x100 ở mốc giữa giây để đáp ứng yêu cầu 500ms */
			if (sub == 0) {
				send_traffic_state(current_phase, (uint8_t)remaining_sec);
			}

			/* Kiểm tra yêu cầu qua đường (nếu có người bấm nút ở Kit B) */
			if (atomic_get(&ped_request_flag) == 1) {
				atomic_set(&ped_request_flag, 0);
				uint8_t req_id = (uint8_t)atomic_get(&ped_req_seq_current);

				struct can_frame resp = {
					.id = CAN_ID_PED_RESP,
					.dlc = 2,
					.flags = 0,
				};
				resp.data[0] = req_id;

				if (current_phase == PHASE_GREEN) {
					/* ĐANG PHA XANH: CHẤP NHẬN YÊU CẦU */
					resp.data[1] = 1; /* 1 = Chấp nhận */
					printf("[%6u ms] [KIT A] CHAP NHAN yeu cau qua duong #%u! (Dang Pha Xanh)\n",
					       k_uptime_get_32(), req_id);

					/* Quy tắc rút ngắn: Bảo đảm Xanh tối thiểu 2s */
					if (elapsed_in_phase < 2) {
						remaining_sec = 2 - elapsed_in_phase;
					} else {
						remaining_sec = 0; /* Chuyển Vàng ngay lập tức */
					}
				} else {
					/* ĐANG PHA ĐỎ HOẶC VÀNG: BỎ QUA */
					resp.data[1] = 0; /* 0 = Bỏ qua */
					printf("[%6u ms] [KIT A] BO QUA yeu cau qua duong #%u (Khong phai Pha Xanh)\n",
					       k_uptime_get_32(), req_id);
				}

				/* Gửi frame phản hồi 0x111 về Kit B */
				can_send(can_dev, &resp, K_MSEC(100), tx_done_cb, NULL);
				send_traffic_state(current_phase, (uint8_t)remaining_sec);
			}
		}

		elapsed_in_phase++;
		remaining_sec--;

		/* Hết thời gian của pha hiện tại -> Chuyển sang pha kế tiếp */
		if (remaining_sec <= 0) {
			elapsed_in_phase = 0;
			switch (current_phase) {
			case PHASE_RED:
				current_phase = PHASE_GREEN;
				phase_duration = 5; /* Xanh 5s */
				break;
			case PHASE_GREEN:
				current_phase = PHASE_YELLOW;
				phase_duration = 2; /* Vàng 2s */
				break;
			case PHASE_YELLOW:
				current_phase = PHASE_RED;
				phase_duration = 7; /* Đỏ 7s */
				break;
			}
			remaining_sec = phase_duration;

			printf("[%6u ms] [KIT A] >>> CHUYEN PHA: %s (%d giay) <<<\n",
			       k_uptime_get_32(),
			       current_phase == PHASE_RED ? "DO" :
			       (current_phase == PHASE_GREEN ? "XANH" : "VANG"),
			       remaining_sec);

			/* Gửi ngay frame trạng thái khi vừa đổi pha */
			send_traffic_state(current_phase, (uint8_t)remaining_sec);
		}
	}
}
K_THREAD_DEFINE(kit_a_fsm_tid, 1024, thread_kit_a_fsm, NULL, NULL, NULL, 5, 0, 500);

#endif /* KIT A */

/*
 * ============================================================================
 * PHẦN DÀNH CHO KIT B: BỘ HIỂN THỊ, NÚT BẤM VÀ FAIL-SAFE WATCHDOG
 * ============================================================================
 */
#if (NODE_ROLE == ROLE_KIT_B) || (DUAL_ROLE_LOOPBACK == 1)

static atomic_t last_rx_state_uptime = ATOMIC_INIT(0);
static atomic_t fail_safe_active = ATOMIC_INIT(0);

/*
 * Thread hiển thị đèn trên Kit B:
 * - Khi hoạt động bình thường: Y0/Y1 theo lệnh từ frame 0x100
 * - Khi mất liên lạc quá 2s: Vàng nhấp nháy (Y0+Y1 cùng chớp 500ms ON / 500ms OFF)
 */
static void thread_kit_b_display(void *p1, void *p2, void *p3)
{
	while (1) {
		uint32_t now = k_uptime_get_32();
		uint32_t last_rx = (uint32_t)atomic_get(&last_rx_state_uptime);

		/* KIỂM TRA ĐIỀU KIỆN FAIL-SAFE (QUÁ 2 GIÂY MẤT TÍN HIỆU 0x100) */
		if (last_rx > 0 && (now - last_rx >= 2000)) {
			if (atomic_get(&fail_safe_active) == 0) {
				atomic_set(&fail_safe_active, 1);
				printf("\n[%6u ms] !!! [FAIL-SAFE ACTIVATED] Mat tin hieu CAN tu Kit A > 2s! Chuyen sang VANG NHAP NHAY !!!\n\n", now);
			}

			/* Chế độ Fail-safe: Cả Y0 và Y1 cùng nháy 500ms BẬT / 500ms TẮT */
			gpio_pin_set_dt(&y0, 1);
			gpio_pin_set_dt(&y1, 1);
			k_msleep(500);

			gpio_pin_set_dt(&y0, 0);
			gpio_pin_set_dt(&y1, 0);
			k_msleep(500);
		} else {
			/* Hoạt động bình thường -> Nghỉ ngắn để nhường CPU */
			k_msleep(50);
		}
	}
}
K_THREAD_DEFINE(kit_b_disp_tid, 1024, thread_kit_b_display, NULL, NULL, NULL, 6, 0, 500);

/*
 * Thread giả lập nút bấm người đi bộ ở Kit B:
 * Mô phỏng lần lượt ở các mốc thời gian: 10s đầu tiên, sau đó lặp lại chu kỳ 14s, 8s, 17s.
 */
static void thread_kit_b_ped_button(void *p1, void *p2, void *p3)
{
	const uint32_t delays_sec[] = {10, 14, 8, 17};
	int step = 0;
	uint8_t req_seq = 1;

	while (1) {
		uint32_t wait_ms = delays_sec[step] * 1000;
		k_msleep(wait_ms);

		struct can_frame req = {
			.id = CAN_ID_PED_REQ,
			.dlc = 1,
			.flags = 0,
		};
		req.data[0] = req_seq;

		printf("\n[%6u ms] [KIT B - NUT QUA DUONG] Nguoi di bo BAM NUT! Gui yeu cau 0x110 (seq=%u)\n",
		       k_uptime_get_32(), req_seq);

		can_send(can_dev, &req, K_MSEC(100), tx_done_cb, NULL);

		req_seq++;
		step = (step + 1) % ARRAY_SIZE(delays_sec);
	}
}
K_THREAD_DEFINE(kit_b_ped_tid, 1024, thread_kit_b_ped_button, NULL, NULL, NULL, 6, 0, 1000);

#endif /* KIT B */

/*
 * ============================================================================
 * THREAD PHÂN PHỐI VÀ XỬ LÝ FRAME CAN NHẬN ĐƯỢC (RX DISPATCHER):
 * ============================================================================
 */
static void thread_traffic_rx_dispatcher(void *p1, void *p2, void *p3)
{
	struct can_frame f;

	while (1) {
		k_msgq_get(&traffic_rx_msgq, &f, K_FOREVER);
		uint32_t now = k_uptime_get_32();
		(void)now; /* Tranh warning unused variable khi build role Kit A */

		/* 1. Xử lý Frame 0x100: Trạng thái đèn từ Kit A gửi tới Kit B */
		if (f.id == CAN_ID_TRAFFIC_STATE) {
#if (NODE_ROLE == ROLE_KIT_B) || (DUAL_ROLE_LOOPBACK == 1)
			uint8_t phase         = f.data[0];
			uint8_t remaining_sec = f.data[1];
			uint8_t seq           = f.data[2];

			/* Cập nhật mốc thời gian nhận tin nhắn */
			atomic_set(&last_rx_state_uptime, (long)now);

			/* Nếu đang ở chế độ Fail-safe mà có tín hiệu lại -> Phục hồi bình thường */
			if (atomic_get(&fail_safe_active) == 1) {
				atomic_set(&fail_safe_active, 0);
				printf("\n[%6u ms] >>> [RECOVERED] Da nhan lai tin hieu tu Kit A! Tro ve binh thuong <<<\n\n", now);
			}

			/* Điều khiển các ngõ ra Y0 / Y1 theo quy ước:
			 * - ĐỎ:   Y0 = 1, Y1 = 0
			 * - XANH: Y0 = 0, Y1 = 1
			 * - VÀNG: Y0 = 1, Y1 = 1
			 */
			if (phase == PHASE_RED) {
				gpio_pin_set_dt(&y0, 1);
				gpio_pin_set_dt(&y1, 0);
			} else if (phase == PHASE_GREEN) {
				gpio_pin_set_dt(&y0, 0);
				gpio_pin_set_dt(&y1, 1);
			} else if (phase == PHASE_YELLOW) {
				gpio_pin_set_dt(&y0, 1);
				gpio_pin_set_dt(&y1, 1);
			}

			printf("[%6u ms] [KIT B RX 0x100] Pha: %-5s | Con lai: %2u giay | seq=%u | Den: [Y0=%u, Y1=%u]\n",
			       now,
			       phase == PHASE_RED ? "DO" : (phase == PHASE_GREEN ? "XANH" : "VANG"),
			       remaining_sec, seq,
			       (phase == PHASE_RED || phase == PHASE_YELLOW) ? 1 : 0,
			       (phase == PHASE_GREEN || phase == PHASE_YELLOW) ? 1 : 0);
#endif
		}
		/* 2. Xử lý Frame 0x110: Yêu cầu người đi bộ từ Kit B gửi tới Kit A */
		else if (f.id == CAN_ID_PED_REQ) {
#if (NODE_ROLE == ROLE_KIT_A) || (DUAL_ROLE_LOOPBACK == 1)
			uint8_t req_seq = f.data[0];
			atomic_set(&ped_req_seq_current, req_seq);
			atomic_set(&ped_request_flag, 1);
#endif
		}
		/* 3. Xử lý Frame 0x111: Phản hồi từ Kit A gửi về cho Kit B */
		else if (f.id == CAN_ID_PED_RESP) {
#if (NODE_ROLE == ROLE_KIT_B) || (DUAL_ROLE_LOOPBACK == 1)
			uint8_t req_seq = f.data[0];
			uint8_t accepted = f.data[1];
			printf("[%6u ms] [KIT B RX 0x111] Phan hoi tu Kit A cho Yeu cau #%u: %s\n",
			       now, req_seq,
			       accepted ? "DUOC CHAP NHAN (Rut ngan Xanh)" : "BI BO QUA");
#endif
		}
	}
}
K_THREAD_DEFINE(traffic_rx_tid, 1024, thread_traffic_rx_dispatcher, NULL, NULL, NULL, 4, 0, 500);

int main(void)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y1, GPIO_OUTPUT_INACTIVE);

	printf("\n=======================================================\n");
	printf("BKIT ARM4 - Lab 3 - Bai 5: Den Giao Thong Phan Tan Qua CAN\n");
	printf("Vai tro: %s | Che do: %s\n",
	       DUAL_ROLE_LOOPBACK ? "Dual Role (Kit A + Kit B Loopback)" :
	       (NODE_ROLE == ROLE_KIT_A ? "KIT A (Controller)" : "KIT B (Display)"),
	       DUAL_ROLE_LOOPBACK ? "Loopback" : "Bus That (CAN Normal)");
	printf("=======================================================\n");

	if (!device_is_ready(can_dev)) {
		printf("Loi: Thiet bi CAN chua san sang!\n");
		return 0;
	}

	/* 1. Chọn chế độ hoạt động */
	can_set_mode(can_dev, DUAL_ROLE_LOOPBACK ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL);

	/*
	 * 2. Cài đặt các bộ lọc nhận frame:
	 * - Frame 0x100: Trạng thái đèn
	 * - Frame 0x110: Yêu cầu người đi bộ
	 * - Frame 0x111: Phản hồi yêu cầu
	 */
	const struct can_filter f_state = {
		.id = CAN_ID_TRAFFIC_STATE,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	const struct can_filter f_ped_req = {
		.id = CAN_ID_PED_REQ,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	const struct can_filter f_ped_resp = {
		.id = CAN_ID_PED_RESP,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	can_add_rx_filter_msgq(can_dev, &traffic_rx_msgq, &f_state);
	can_add_rx_filter_msgq(can_dev, &traffic_rx_msgq, &f_ped_req);
	can_add_rx_filter_msgq(can_dev, &traffic_rx_msgq, &f_ped_resp);

	/* 3. Khởi động CAN controller */
	if (can_start(can_dev) != 0) {
		printf("Loi can_start!\n");
		return 0;
	}
	printf("CAN Controller da khoi dong thanh cong!\n\n");

	return 0;
}
