/*
 * ports/linux/port_linux.h - Linux SocketCAN implementation of hal/hal.h (ND-1).
 *
 * Raw CAN socket (PF_CAN / SOCK_RAW / CAN_RAW), non-blocking, single-threaded:
 * gbt_hal_can_receive() reads at most one frame per call (the integration
 * loop bounds the count). Classical CAN only; 29-bit and 11-bit identifiers.
 * RX drops are taken from the kernel socket-queue counter (SO_RXQ_OVFL) and
 * reported as rx_overflow_hw. Error frames (CAN_RAW_ERR_FILTER) update the
 * controller state; vcan produces none. Must not include app/ or protocol/.
 */
#ifndef GBT_PORT_LINUX_H
#define GBT_PORT_LINUX_H

#include <stdint.h>
#include <linux/can.h>
#include "hal/hal.h"
#include "ports/common/gbt_hal_lc.h"

#define GBT_PORT_LINUX_IFNAME_MAX 16u

struct gbt_hal {
    uint8_t           lifecycle;          /* GBT_HAL_LC_* */
    int               sock_fd;            /* -1 when closed */
    char              ifname[GBT_PORT_LINUX_IFNAME_MAX];
    uint32_t          accepted_seq;
    uint8_t           in_sink;
    uint32_t          kernel_drops_last;  /* last SO_RXQ_OVFL value */
    uint32_t          error_frames;
    uint32_t          event_count;
    uint16_t          last_event_code;
    gbt_can_status_t  status;
    gbt_output_sink_t sink;
};

typedef struct {
    const char *ifname;                   /* e.g. "vcan0" */
} gbt_port_linux_cfg_t;

/* UNINIT -> READY: open, bind, non-blocking. GBT_HAL_E_IO on any socket error
 * (details in *err_out if non-NULL: errno value). */
gbt_hal_status_t gbt_port_linux_init(gbt_hal_t *h, const gbt_port_linux_cfg_t *cfg, int *err_out);
/* READY -> UNINIT: close the socket. */
gbt_hal_status_t gbt_port_linux_deinit(gbt_hal_t *h);

/* Frame conversion (exposed for unit tests). Return 0 if not convertible. */
uint8_t gbt_port_linux_to_can(const gbt_can_frame_t *in, struct can_frame *out);
uint8_t gbt_port_linux_from_can(const struct can_frame *in, gbt_can_frame_t *out);

#endif /* GBT_PORT_LINUX_H */
