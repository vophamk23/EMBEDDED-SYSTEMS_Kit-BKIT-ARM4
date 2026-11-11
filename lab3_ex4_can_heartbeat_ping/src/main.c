/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * BÀI TẬP 4: HEARTBEAT, PING-PONG VÀ GIÁM SÁT NODE TRÊN MẠNG CAN
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI TẬP:
 *    - Triển khai mạng CAN 2 node vật lý thực tế (Kit A và Kit B):
 *      + Kit A nạp code với NODE_ID = 1.
 *      + Kit B nạp code với NODE_ID = 2.
 *      + Nối CANH với CANH, CANL với CANL, nối chung GND, gạt SW13 = ON trên cả 2 kit.
 *      + (Hỗ trợ SELF_TEST = 1 để kiểm tra loopback trên 1 kit nếu thiếu thiết bị).
 *    - Cơ chế Heartbeat (Nhịp tim) 1s và Watchdog 3s:
 *      + Mỗi node phát Heartbeat định kỳ 1 giây: ID 0x700 + NODE_ID.
 *      + Đối tác giám sát qua Semaphore (partner_sem) với timeout 3 giây.
 *      + Quá 3 giây không nhận được -> In "PARTNER LOST", BẬT LED3.
 *      + Khi nhận lại được -> In "PARTNER BACK", TẮT LED3.
 *      + Mỗi frame Heartbeat nhận được làm đảo trạng thái LED Y0 (PE5).
 *    - Cơ chế PING - PONG đo RTT (Round-Trip Time):
 *      + Mỗi 5 giây phát gói PING 0x600 + NODE_ID mang mốc thời gian uptime 32-bit.
 *      + Đối tác nhận được lập tức phản hồi PONG 0x680 + NODE_ID_của_người_ping mang nguyên vẹn payload.
 *      + Người gửi nhận PONG, tính toán và in thời gian khứ hồi RTT (ms).
 *    - Giám sát trạng thái vật lý của mạng:
 *      + Đăng ký can_set_state_change_callback() ghi log TEC/REC khi có sự cố đường truyền
 *        (ví dụ khi rút dây CANH hoặc tắt nguồn một node).
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* CẤU HÌNH NODE_ID: 1 cho Kit A, 2 cho Kit B */
#define NODE_ID   1

/* CẤU HÌNH CHẾ ĐỘ: 0 = Bus vật lý thật 2 kit, 1 = Loopback nội tại 1 kit */
#define SELF_TEST 0

#if SELF_TEST
#define PARTNER_ID NODE_ID
#else
#define PARTNER_ID ((NODE_ID == 1) ? 2 : 1)
#endif

/* Các dải ID quy định */
#define HB_BASE_TX   (0x700 + NODE_ID)
#define HB_BASE_RX   (0x700 + PARTNER_ID)
#define PING_BASE_TX (0x600 + NODE_ID)
#define PING_BASE_RX (0x600 + PARTNER_ID)
#define PONG_BASE_TX (0x680 + PARTNER_ID)
#define PONG_BASE_RX (0x680 + NODE_ID)

/* Thiết bị CAN và GPIO */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios); /* PE4: Báo mất kết nối */
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios); /* PE5: Đảo khi nhận HB */

/* Semaphore báo hiệu nhận được heartbeat từ đối tác */
K_SEM_DEFINE(partner_sem, 0, 1);

/* Message Queue nhận frame PING/PONG và Heartbeat */
CAN_MSGQ_DEFINE(can_rx_msgq, 16);

/* Cấu trúc và hàng đợi giám sát trạng thái bus */
struct state_evt {
	enum can_state state;
	struct can_bus_err_cnt err;
};
K_MSGQ_DEFINE(state_q, sizeof(struct state_evt), 8, 4);

static const char *state_name(enum can_state s)
{
	switch (s) {
	case CAN_STATE_ERROR_ACTIVE:  return "ERROR-ACTIVE (Binh thuong)";
	case CAN_STATE_ERROR_WARNING: return "ERROR-WARNING (Canh bao loi)";
	case CAN_STATE_ERROR_PASSIVE: return "ERROR-PASSIVE (Thu dong)";
	case CAN_STATE_BUS_OFF:       return "BUS-OFF (Mat ket noi!)";
	default:                      return "STOPPED";
	}
}

static void state_change_cb(const struct device *dev, enum can_state state,
			    struct can_bus_err_cnt err_cnt, void *user_data)
{
	struct state_evt e = { .state = state, .err = err_cnt };
	k_msgq_put(&state_q, &e, K_NO_WAIT);
}

/* Callback khi hoàn thành gửi */
static void tx_done_cb(const struct device *dev, int error, void *user_data)
{
	/* Không thao tác trong ISR */
}

/*
 * ============================================================================
 * THREAD PHÁT HEARTBEAT (MỖI 1 GIÂY):
 * Gửi frame 0x700 + NODE_ID, DLC = 3 [NODE_ID, seq, current_can_state].
 * ============================================================================
 */
static void thread_heartbeat_tx(void *p1, void *p2, void *p3)
{
	struct can_frame f = {
		.id = HB_BASE_TX,
		.dlc = 3,
		.flags = 0,
	};
	uint8_t seq = 0;

	while (1) {
		enum can_state state;
		struct can_bus_err_cnt err;
		can_get_state(can_dev, &state, &err);

		f.data[0] = NODE_ID;
		f.data[1] = seq++;
		f.data[2] = (uint8_t)state;

		/* can_send với timeout 100ms và callback để không treo khi đứt cáp */
		int ret = can_send(can_dev, &f, K_MSEC(100), tx_done_cb, NULL);
		if (ret != 0) {
			printf("[%6u ms] [TX HB LOI] can_send that bai: %d\n", k_uptime_get_32(), ret);
		}

		k_msleep(1000);
	}
}
K_THREAD_DEFINE(hb_tx_tid, 1024, thread_heartbeat_tx, NULL, NULL, NULL, 6, 0, 500);

/*
 * ============================================================================
 * THREAD PHÁT PING (MỖI 5 GIÂY):
 * Gửi frame 0x600 + NODE_ID mang timestamp 32-bit (Big-Endian).
 * ============================================================================
 */
static void thread_ping_tx(void *p1, void *p2, void *p3)
{
	struct can_frame f = {
		.id = PING_BASE_TX,
		.dlc = 4,
		.flags = 0,
	};

	while (1) {
		k_msleep(5000);

		uint32_t t_now = k_uptime_get_32();
		f.data[0] = (uint8_t)((t_now >> 24) & 0xFF);
		f.data[1] = (uint8_t)((t_now >> 16) & 0xFF);
		f.data[2] = (uint8_t)((t_now >> 8) & 0xFF);
		f.data[3] = (uint8_t)(t_now & 0xFF);

		printf("[%6u ms] [PING] Gui PING toi Node %d (t_send=%u ms)...\n",
		       t_now, PARTNER_ID, t_now);

		can_send(can_dev, &f, K_MSEC(100), tx_done_cb, NULL);
	}
}
K_THREAD_DEFINE(ping_tx_tid, 1024, thread_ping_tx, NULL, NULL, NULL, 6, 0, 1000);

/*
 * ============================================================================
 * THREAD XỬ LÝ FRAME CAN NHẬN ĐƯỢC (RX DISPATCHER):
 * - Heartbeat từ đối tác -> give partner_sem, đảo LED Y0.
 * - PING từ đối tác -> gửi lại PONG ngay lập tức.
 * - PONG trả về -> tính và in RTT (ms).
 * ============================================================================
 */
static void thread_rx_dispatcher(void *p1, void *p2, void *p3)
{
	struct can_frame f;

	while (1) {
		k_msgq_get(&can_rx_msgq, &f, K_FOREVER);
		uint32_t now = k_uptime_get_32();

		/* 1. Xử lý Heartbeat từ Node đối tác */
		if (f.id == HB_BASE_RX) {
			uint8_t sender = f.data[0];
			uint8_t seq    = f.data[1];
			uint8_t state  = f.data[2];

			printf("[%6u ms] [RX HB] Tu Node %u | seq=%u | Trang thai=%u\n",
			       now, sender, seq, state);

			/* Đánh thức watchdog */
			k_sem_give(&partner_sem);
			/* Đảo trạng thái LED Y0 */
			gpio_pin_toggle_dt(&y0);
		}
		/* 2. Xử lý yêu cầu PING từ Node đối tác -> Gửi trả PONG */
		else if (f.id == PING_BASE_RX) {
			struct can_frame pong = {
				.id = PONG_BASE_TX,
				.dlc = 4,
				.flags = 0,
			};
			/* Sao chép nguyên vẹn 4 byte payload của gói PING */
			for (int i = 0; i < 4; i++) {
				pong.data[i] = f.data[i];
			}

			printf("[%6u ms] [PONG] Nhan PING tu Node %d -> Gui tra PONG 0x%03x\n",
			       now, PARTNER_ID, pong.id);
			can_send(can_dev, &pong, K_MSEC(100), tx_done_cb, NULL);
		}
		/* 3. Xử lý nhận gói PONG phản hồi cho PING của mình */
		else if (f.id == PONG_BASE_RX) {
			uint32_t t_send = ((uint32_t)f.data[0] << 24) |
			                  ((uint32_t)f.data[1] << 16) |
			                  ((uint32_t)f.data[2] << 8)  |
			                  ((uint32_t)f.data[3]);
			uint32_t rtt = (now >= t_send) ? (now - t_send) : 0;

			printf("[%6u ms] >>> [PING-PONG THANH CONG] RTT voi Node %d = %u ms <<<\n",
			       now, PARTNER_ID, rtt);
		}
	}
}
K_THREAD_DEFINE(rx_disp_tid, 1024, thread_rx_dispatcher, NULL, NULL, NULL, 5, 0, 500);

/*
 * ============================================================================
 * THREAD WATCHDOG GIÁM SÁT ĐỐI TÁC (3 GIÂY TIMEOUT):
 * ============================================================================
 */
static void thread_watchdog(void *p1, void *p2, void *p3)
{
	bool partner_alive = true;

	while (1) {
		/* Chờ semaphore với thời gian hết hạn 3 giây */
		int ret = k_sem_take(&partner_sem, K_SECONDS(3));

		if (ret == 0) {
			/* Nhận được nhịp tim thành công */
			if (!partner_alive) {
				partner_alive = true;
				gpio_pin_set_dt(&led3, 0); /* Tắt LED3 báo lỗi */
				printf("\n[%6u ms] >>> PARTNER BACK <<< (Node %d da hoat dong tro lai!)\n\n",
				       k_uptime_get_32(), PARTNER_ID);
			}
		} else {
			/* Hết hạn 3 giây mà không có heartbeat */
			if (partner_alive) {
				partner_alive = false;
				gpio_pin_set_dt(&led3, 1); /* BẬT LED3 báo động mất kết nối */
				printf("\n[%6u ms] !!! PARTNER LOST !!! (Khong co tin hieu tu Node %d trong 3 giay!)\n\n",
				       k_uptime_get_32(), PARTNER_ID);
			}
		}
	}
}
K_THREAD_DEFINE(watchdog_tid, 1024, thread_watchdog, NULL, NULL, NULL, 5, 0, 1000);

/*
 * ============================================================================
 * THREAD GHI LOG TRẠNG THÁI BUS:
 * ============================================================================
 */
static void thread_state_log(void *p1, void *p2, void *p3)
{
	struct state_evt e;

	while (1) {
		k_msgq_get(&state_q, &e, K_FOREVER);
		printf("[%6u ms] [CAN TRANG THAI] %s | TEC=%u, REC=%u\n",
		       k_uptime_get_32(), state_name(e.state),
		       e.err.tx_err_cnt, e.err.rx_err_cnt);
	}
}
K_THREAD_DEFINE(state_log_tid, 1024, thread_state_log, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

	printf("\n=======================================================\n");
	printf("BKIT ARM4 - Lab 3 - Bai 4: CAN Heartbeat, Ping & Watchdog\n");
	printf("Node dang chay: NODE_ID = %d | Doi tac: PARTNER_ID = %d\n", NODE_ID, PARTNER_ID);
	printf("Che do: %s | Toc do: 500 kbit/s\n", SELF_TEST ? "Loopback (1 Kit)" : "Bus That (2 Kit)");
	printf("=======================================================\n");

	if (!device_is_ready(can_dev)) {
		printf("Loi: Thiet bi CAN chua san sang!\n");
		return 0;
	}

	/* 1. Chọn chế độ hoạt động (NORMAL cho bus thật) */
	can_set_mode(can_dev, SELF_TEST ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL);

	/* 2. Đăng ký callback giám sát thay đổi trạng thái đường truyền */
	can_set_state_change_callback(can_dev, state_change_cb, NULL);

	/*
	 * 3. Cài đặt các bộ lọc nhận:
	 * - Nhận Heartbeat từ đối tác: HB_BASE_RX
	 * - Nhận PING từ đối tác: PING_BASE_RX
	 * - Nhận PONG phản hồi: PONG_BASE_RX
	 */
	const struct can_filter f_hb = {
		.id = HB_BASE_RX,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	const struct can_filter f_ping = {
		.id = PING_BASE_RX,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	const struct can_filter f_pong = {
		.id = PONG_BASE_RX,
		.mask = CAN_STD_ID_MASK,
		.flags = 0,
	};
	can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &f_hb);
	can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &f_ping);
	can_add_rx_filter_msgq(can_dev, &can_rx_msgq, &f_pong);

	/* 4. Khởi động bộ điều khiển CAN */
	if (can_start(can_dev) != 0) {
		printf("Loi can_start!\n");
		return 0;
	}
	printf("CAN Controller da khoi dong! Dang tham gia vao bus...\n\n");

	return 0;
}
