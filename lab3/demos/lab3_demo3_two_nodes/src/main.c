/*
 * ============================================================================
 * BKIT ARM4 - HỆ THỐNG NHÚNG - LAB 3: GIAO TIẾP CAN-BUS
 * DEMO 3: GIAO TIẾP HAI NODE TRÊN BUS THẬT VÀ GIÁM SÁT TRẠNG THÁI BUS
 * ============================================================================
 * 
 * 1. MỤC TIÊU BÀI DEMO:
 *    - Triển khai giao tiếp CAN thực tế giữa HAI kit vi điều khiển (Kit A và Kit B):
 *      + Nối chân CANH (Kit A) với CANH (Kit B).
 *      + Nối chân CANL (Kit A) với CANL (Kit B).
 *      + Nối GND chung giữa hai kit để bảo đảm điện áp ngưỡng vi sai chuẩn xác.
 *      + BẬT công tắc SW13 trên cả hai kit để đóng điện trở kết thúc đường dây 120 Ohm.
 *    - Thiết kế cơ chế Heartbeat (Nhịp tim) định kỳ:
 *      + Mỗi node định kỳ 1 giây phát một frame Heartbeat (0x700 + NODE_ID).
 *      + Node đối tác giám sát nhịp tim với cơ chế Timeout (3 giây). Nếu quá 3 giây không
 *        nhận được, node phát hiện đối tác đã bị mất nguồn hoặc đứt cáp.
 *    - Truyền bất đồng bộ an toàn trên bus thật:
 *      + Sử dụng can_send kèm TX Callback và timeout K_MSEC(100).
 *      + Tuyệt đối KHÔNG truyền đồng bộ (callback = NULL, K_FOREVER) trên bus thật vì nếu
 *        node kia vắng mặt (không có ACK bit) hoặc bus bị ngắn mạch, hàm sẽ treo thread mãi mãi!
 *    - Giám sát trạng thái vật lý của mạng CAN (CAN Bus State Machine):
 *      + Sử dụng can_set_state_change_callback() để nhận biết các biến đổi trạng thái:
 *        * ERROR-ACTIVE: Trạng thái bình thường, bộ đếm lỗi TEC, REC < 96.
 *        * ERROR-WARNING: Bộ đếm lỗi TEC hoặc REC >= 96.
 *        * ERROR-PASSIVE: Lỗi nghiêm trọng, TEC hoặc REC > 127 (chỉ được gửi cờ lỗi bị động).
 *        * BUS-OFF: TEC > 255, nút tự ngắt hoàn toàn khỏi mạng để bảo vệ bus.
 *
 * 2. CẤU HÌNH BIÊN DỊCH VÀ NẠP PHẦN CỨNG:
 *    - Kit A: Để nguyên #define NODE_ID 1, #define SELF_TEST 0 -> Build & Flash vào Kit A.
 *    - Kit B: Sửa thành #define NODE_ID 2, #define SELF_TEST 0 -> Build & Flash vào Kit B.
 *    - Chế độ tự kiểm tra (Chỉ có 1 kit): Đặt #define SELF_TEST 1 (Chạy loopback tự gửi tự nhận).
 * ============================================================================
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>

/* Định danh Node: Đổi thành 1 khi nạp Kit A, đổi thành 2 khi nạp Kit B */
#define NODE_ID   1

/* Chế độ kiểm tra: 0 = Bus vật lý thật 2 kit, 1 = Loopback nội tại 1 kit */
#define SELF_TEST 0

/* ID cơ sở của khung Heartbeat */
#define HB_BASE   0x700

/* Lấy thiết bị CAN và các chân GPIO từ Devicetree */
static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios); /* Báo lỗi Bus */
static const struct gpio_dt_spec y0   = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios); /* Đảo khi nhận HB */

/* Hàng đợi chứa frame Heartbeat nhận được */
CAN_MSGQ_DEFINE(hb_msgq, 8);

/*
 * Cấu trúc và hàng đợi lưu sự kiện thay đổi trạng thái CAN bus:
 * Truyền từ Callback ngắt (state_change_cb) sang thread ghi log (thread_state_log).
 */
struct state_evt {
	enum can_state state;
	struct can_bus_err_cnt err;
};
K_MSGQ_DEFINE(state_q, sizeof(struct state_evt), 8, 4);

/**
 * @brief Chuyển đổi mã trạng thái CAN sang chuỗi ký tự hiển thị
 */
static const char *state_name(enum can_state s)
{
	switch (s) {
	case CAN_STATE_ERROR_ACTIVE:  return "ERROR-ACTIVE (Binh thuong)";
	case CAN_STATE_ERROR_WARNING: return "ERROR-WARNING (Canh bao loi)";
	case CAN_STATE_ERROR_PASSIVE: return "ERROR-PASSIVE (Thu dong/Bi dong)";
	case CAN_STATE_BUS_OFF:       return "BUS-OFF (Cach ly khoi bus!)";
	default:                      return "STOPPED (Dung)";
	}
}

/*
 * Callback giám sát thay đổi trạng thái CAN (chạy trong ngữ cảnh ngắt ISR):
 * Được gọi tự động mỗi khi trạng thái mạng thay đổi hoặc bộ đếm lỗi biến động.
 */
static void state_change_cb(const struct device *dev, enum can_state state,
			    struct can_bus_err_cnt err_cnt, void *user_data)
{
	struct state_evt e = {
		.state = state,
		.err = err_cnt
	};

	/* Đẩy vào message queue với K_NO_WAIT để thoát ngắt ngay lập tức */
	k_msgq_put(&state_q, &e, K_NO_WAIT);
}

/* Biến nguyên tử lưu mã lỗi truyền frame gần nhất */
static atomic_t last_tx_err;

/*
 * Callback khi truyền xong 1 frame (chạy trong ngữ cảnh ngắt ISR):
 * error = 0 nếu truyền thành công và nhận được ACK bit từ bus;
 * error < 0 (như -EAGAIN, -EIO) nếu truyền thất bại hoặc timeout.
 */
static void tx_done_cb(const struct device *dev, int error, void *user_data)
{
	atomic_set(&last_tx_err, error);
}

/*
 * ============================================================================
 * THREAD PHÁT HEARTBEAT (TX):
 * Định kỳ mỗi 1 giây đóng gói NODE_ID và số thứ tự seq gửi lên bus.
 * ============================================================================
 */
static void thread_heartbeat_tx(void *p1, void *p2, void *p3)
{
	struct can_frame f = {
		.id = HB_BASE + NODE_ID,
		.dlc = 2,
		.flags = 0,
	};
	uint8_t seq = 0;

	while (1) {
		f.data[0] = NODE_ID;
		f.data[1] = seq++;

		/*
		 * Truyền bất đồng bộ:
		 * - timeout = K_MSEC(100): Nếu 3 Mailbox TX đầy (do mất ACK), chờ tối đa 100ms.
		 * - tx_done_cb: Nhận thông báo khi gửi xong.
		 * Giúp thread không bao giờ bị nghẽn (hung thread) ngay cả khi đứt bus.
		 */
		int ret = can_send(can_dev, &f, K_MSEC(100), tx_done_cb, NULL);

		if (ret != 0) {
			printf("[%6u ms] [TX FAIL] Khong the dua frame vao TX Mailbox: %d\n",
			       k_uptime_get_32(), ret);
		}

		k_msleep(1000);
	}
}
K_THREAD_DEFINE(hb_tx_tid, 1024, thread_heartbeat_tx, NULL, NULL, NULL, 6, 0, 500);

/*
 * ============================================================================
 * THREAD NHẬN HEARTBEAT VÀ GIÁM SÁT ĐỐI TÁC (RX & WATCHDOG):
 * Chờ đợi frame Heartbeat từ đối tác trong tối đa 3 giây.
 * ============================================================================
 */
static void thread_heartbeat_rx(void *p1, void *p2, void *p3)
{
	struct can_frame f;

	while (1) {
		/*
		 * k_msgq_get có timeout K_SECONDS(3):
		 * Nếu trong 3 giây có frame -> trả về 0 (đối tác còn sống).
		 * Nếu hết 3 giây không có frame -> trả về -EAGAIN (đối tác mất tín hiệu).
		 */
		if (k_msgq_get(&hb_msgq, &f, K_SECONDS(3)) == 0) {
			printf("[%6u ms] [RX HB] Nhan Heartbeat tu Node %u | seq=%u\n",
			       k_uptime_get_32(), f.data[0], f.data[1]);
			/* Đảo trạng thái LED Y0 (PE5) báo nhận nhịp tim thành công */
			gpio_pin_toggle_dt(&y0);
		} else {
			printf("[%6u ms] [CANH BAO MAT LIEN LAC] Khong nhan duoc Heartbeat trong 3 giay!\n",
			       k_uptime_get_32());
		}
	}
}
K_THREAD_DEFINE(hb_rx_tid, 1024, thread_heartbeat_rx, NULL, NULL, NULL, 5, 0, 500);

/*
 * ============================================================================
 * THREAD GHI LOG TRẠNG THÁI BUS (DIAGNOSTIC & LOGGING):
 * Hiển thị giá trị bộ đếm lỗi TEC (Transmit Error Counter) và REC (Receive Error Counter).
 * ============================================================================
 */
static void thread_state_log(void *p1, void *p2, void *p3)
{
	struct state_evt e;

	while (1) {
		k_msgq_get(&state_q, &e, K_FOREVER);
		printf("[%6u ms] [BUS STATE] %s | TEC=%u, REC=%u\n",
		       k_uptime_get_32(), state_name(e.state),
		       e.err.tx_err_cnt, e.err.rx_err_cnt);

		/* Bật LED3 (PE4) cảnh báo khi bus rơi vào trạng thái có lỗi (khác ERROR_ACTIVE) */
		gpio_pin_set_dt(&led3, e.state != CAN_STATE_ERROR_ACTIVE);
	}
}
K_THREAD_DEFINE(state_tid, 1024, thread_state_log, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	/*
	 * Cấu hình Bộ lọc nhận Heartbeat:
	 * Nhận dải ID 0x700 .. 0x70F (ID = 0x700, MASK = 0x7F0)
	 * Nhờ đó một node có thể lắng nghe Heartbeat của mọi node khác trên mạng.
	 */
	const struct can_filter hb_filter = {
		.id = HB_BASE,
		.mask = 0x7F0,
		.flags = 0
	};

	/* Khởi tạo các chân LED */
	gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&y0, GPIO_OUTPUT_INACTIVE);

	printf("\n======================================================\n");
	printf("BKIT ARM4 - Lab 3 - Demo 3: CAN Bus Hai Node & Giam Sat\n");
	printf("Node dang chay: NODE_ID = %d | Che do: %s\n",
	       NODE_ID, SELF_TEST ? "Loopback (1 Kit)" : "Bus That (2 Kit)");
	printf("======================================================\n");

	if (!device_is_ready(can_dev)) {
		printf("Loi: Thiet bi CAN chua san sang!\n");
		return 0;
	}

	/* BƯỚC 1: Chọn chế độ hoạt động (NORMAL cho bus thật, LOOPBACK cho test nội tại) */
	can_set_mode(can_dev, SELF_TEST ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL);

	/* BƯỚC 2: Đăng ký callback giám sát trạng thái vật lý của đường truyền CAN */
	can_set_state_change_callback(can_dev, state_change_cb, NULL);

	/* BƯỚC 3: Cài đặt bộ lọc phần cứng vào hàng đợi hb_msgq */
	can_add_rx_filter_msgq(can_dev, &hb_msgq, &hb_filter);

	/* BƯỚC 4: Khởi động bộ điều khiển CAN */
	can_start(can_dev);
	printf("CAN Controller da khoi dong! Dang lang nghe bus...\n\n");

	return 0;
}
