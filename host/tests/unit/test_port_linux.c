/*
 * tests/unit/test_port_linux.c - Linux SocketCAN port parts that do not need
 * a CAN interface: frame conversion (29/11-bit, DLC, all 8 bytes, RTR/error
 * rejection), lifecycle and error handling when the interface is missing.
 * The two-process exchange itself is tested by tools/run_linux_sil.sh.
 */
#include "ports/linux/port_linux.h"
#include "tests/support/test_assert.h"

#include <string.h>

static void test_conversion(void)
{
    gbt_can_frame_t g;
    gbt_can_frame_t back;
    struct can_frame c;
    unsigned i;

    memset(&g, 0, sizeof g);
    g.id = 0x1826F456u;
    g.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    g.dlc = 3U;
    for (i = 0U; i < 8U; i++) {
        g.data[i] = (uint8_t)(0xA0U + i);
    }
    CHECK_EQ_U(gbt_port_linux_to_can(&g, &c), 1U);
    CHECK_EQ_U(c.can_id, 0x1826F456u | CAN_EFF_FLAG);
    CHECK_EQ_U(c.can_dlc, 3U);
    CHECK_EQ_U(c.data[7], 0xA7U);                      /* all 8 bytes carried */
    CHECK_EQ_U(gbt_port_linux_from_can(&c, &back), 1U);
    CHECK(memcmp(&g, &back, sizeof g) == 0);

    g.id = 0x123U;
    g.flags = (uint8_t)GBT_CAN_FLAG_STD;
    g.dlc = 8U;
    CHECK_EQ_U(gbt_port_linux_to_can(&g, &c), 1U);
    CHECK_EQ_U(c.can_id, 0x123U);
    CHECK_EQ_U(gbt_port_linux_from_can(&c, &back), 1U);
    CHECK_EQ_U(back.flags, GBT_CAN_FLAG_STD);

    g.dlc = 9U;
    CHECK_EQ_U(gbt_port_linux_to_can(&g, &c), 0U);      /* invalid DLC */
    g.dlc = 1U;
    g.flags = 0x02U;
    CHECK_EQ_U(gbt_port_linux_to_can(&g, &c), 0U);      /* unknown flag */

    memset(&c, 0, sizeof c);
    c.can_id = 0x1826F456u | CAN_EFF_FLAG | CAN_RTR_FLAG;
    CHECK_EQ_U(gbt_port_linux_from_can(&c, &back), 0U); /* remote frame */
    c.can_id = CAN_ERR_FLAG | 0x40U;
    CHECK_EQ_U(gbt_port_linux_from_can(&c, &back), 0U); /* error frame */
    c.can_id = 0x10U;
    c.can_dlc = 9U;
    CHECK_EQ_U(gbt_port_linux_from_can(&c, &back), 0U); /* DLC > 8 */
}

static void test_missing_interface(void)
{
    static gbt_hal_t h;
    gbt_port_linux_cfg_t cfg;
    gbt_can_frame_t f;
    int err = 0;

    memset(&f, 0, sizeof f);
    cfg.ifname = "gbtnoif0";
    CHECK(gbt_port_linux_init(&h, &cfg, &err) == GBT_HAL_E_IO);
    CHECK(err != 0);
    CHECK_EQ_U(h.lifecycle, GBT_HAL_LC_UNINIT);
    CHECK(gbt_hal_can_send(&h, &f) == GBT_HAL_E_NOT_READY);
    CHECK(gbt_hal_can_receive(&h, &f) == GBT_HAL_E_NOT_READY);
    CHECK_EQ_U(gbt_hal_now_ms(&h), 0U);
    cfg.ifname = "lo";                                   /* exists, but is not a CAN device */
    CHECK(gbt_port_linux_init(&h, &cfg, &err) == GBT_HAL_E_IO);
    cfg.ifname = "";
    CHECK(gbt_port_linux_init(&h, &cfg, &err) == GBT_HAL_E_PARAM);
    cfg.ifname = "an-interface-name-too-long";
    CHECK(gbt_port_linux_init(&h, &cfg, &err) == GBT_HAL_E_PARAM);
    CHECK(gbt_port_linux_deinit(&h) == GBT_HAL_E_STATE);
}

int main(void)
{
    test_conversion();
    test_missing_interface();
    return TEST_SUMMARY("test_port_linux");
}
