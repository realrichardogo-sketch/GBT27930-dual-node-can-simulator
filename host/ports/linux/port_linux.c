/* ports/linux/port_linux.c - see port_linux.h. Linux/POSIX port code only. */
#define _DEFAULT_SOURCE 1             /* POSIX + BSD socket APIs under -std=c99 */

#include "ports/linux/port_linux.h"

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <linux/can/error.h>
#include <linux/can/raw.h>

#ifndef SO_RXQ_OVFL
#define SO_RXQ_OVFL 40
#endif

static void sat_inc(uint32_t *c)
{
    if (*c != 0xFFFFFFFFu) {
        (*c)++;
    }
}

/* ---- conversion ---- */
uint8_t gbt_port_linux_to_can(const gbt_can_frame_t *in, struct can_frame *out)
{
    if (gbt_can_frame_is_valid(in) == 0U || out == NULL) {
        return 0U;
    }
    memset(out, 0, sizeof *out);
    if ((in->flags & GBT_CAN_FLAG_EXT) != 0U) {
        out->can_id = (canid_t)(in->id & CAN_EFF_MASK) | CAN_EFF_FLAG;
    } else {
        out->can_id = (canid_t)(in->id & CAN_SFF_MASK);
    }
    out->can_dlc = in->dlc;
    memcpy(out->data, in->data, sizeof out->data);   /* all 8 bytes preserved */
    return 1U;
}

uint8_t gbt_port_linux_from_can(const struct can_frame *in, gbt_can_frame_t *out)
{
    if (in == NULL || out == NULL) {
        return 0U;
    }
    if ((in->can_id & (CAN_RTR_FLAG | CAN_ERR_FLAG)) != 0U || in->can_dlc > CAN_MAX_DLEN) {
        return 0U;                                     /* remote / error / non-classical */
    }
    memset(out, 0, sizeof *out);
    if ((in->can_id & CAN_EFF_FLAG) != 0U) {
        out->id = in->can_id & CAN_EFF_MASK;
        out->flags = (uint8_t)GBT_CAN_FLAG_EXT;
    } else {
        out->id = in->can_id & CAN_SFF_MASK;
        out->flags = (uint8_t)GBT_CAN_FLAG_STD;
    }
    out->dlc = in->can_dlc;
    memcpy(out->data, in->data, sizeof out->data);
    return 1U;
}

/* ---- port lifecycle ---- */
gbt_hal_status_t gbt_port_linux_init(gbt_hal_t *h, const gbt_port_linux_cfg_t *cfg, int *err_out)
{
    struct ifreq ifr;
    struct sockaddr_can addr;
    can_err_mask_t err_mask = CAN_ERR_MASK;
    int one = 1;
    int fd;
    int flags;

    if (err_out != NULL) {
        *err_out = 0;
    }
    if (h == NULL || cfg == NULL || cfg->ifname == NULL
        || strlen(cfg->ifname) == 0U || strlen(cfg->ifname) >= GBT_PORT_LINUX_IFNAME_MAX) {
        return GBT_HAL_E_PARAM;
    }
    if (h->lifecycle != GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_STATE;
    }
    memset(h, 0, sizeof *h);
    h->sock_fd = -1;
    fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        goto fail;
    }
    memset(&ifr, 0, sizeof ifr);
    strncpy(ifr.ifr_name, cfg->ifname, sizeof ifr.ifr_name - 1U);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        goto fail_close;
    }
    /* error frames: controller state and bus errors (none on vcan) */
    (void)setsockopt(fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof err_mask);
    /* kernel RX-queue drop counter delivered with each frame */
    (void)setsockopt(fd, SOL_SOCKET, SO_RXQ_OVFL, &one, sizeof one);
    memset(&addr, 0, sizeof addr);
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        goto fail_close;
    }
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        goto fail_close;
    }
    h->sock_fd = fd;
    strncpy(h->ifname, cfg->ifname, sizeof h->ifname - 1U);
    h->status.state = (uint8_t)GBT_CAN_STATE_ERROR_ACTIVE;
    h->status.tec_rec_valid = 0U;                      /* not readable through CAN_RAW */
    h->lifecycle = (uint8_t)GBT_HAL_LC_READY;
    return GBT_HAL_OK;

fail_close:
    if (err_out != NULL) {
        *err_out = errno;
    }
    (void)close(fd);
    return GBT_HAL_E_IO;
fail:
    if (err_out != NULL) {
        *err_out = errno;
    }
    return GBT_HAL_E_IO;
}

gbt_hal_status_t gbt_port_linux_deinit(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_READY) {
        return GBT_HAL_E_STATE;
    }
    if (h->sock_fd >= 0) {
        (void)close(h->sock_fd);
    }
    h->sock_fd = -1;
    h->lifecycle = (uint8_t)GBT_HAL_LC_UNINIT;
    return GBT_HAL_OK;
}

/* ---- ND-1 lifecycle (same contract as every port) ---- */
gbt_hal_status_t gbt_hal_bind_output_sink(gbt_hal_t *h, const gbt_output_sink_t *sink)
{
    if (h == NULL || sink == NULL || sink->apply == NULL || sink->feedback == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    if (h->lifecycle != GBT_HAL_LC_READY) {
        return GBT_HAL_E_STATE;
    }
    h->sink = *sink;
    h->lifecycle = (uint8_t)GBT_HAL_LC_BOUND;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_unbind_output_sink(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_BOUND) {
        return GBT_HAL_E_STATE;
    }
    memset(&h->sink, 0, sizeof h->sink);
    h->lifecycle = (uint8_t)GBT_HAL_LC_READY;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_claim(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT || h->lifecycle == GBT_HAL_LC_READY) {
        return GBT_HAL_E_NOT_READY;
    }
    if (h->lifecycle != GBT_HAL_LC_BOUND) {
        return GBT_HAL_E_STATE;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_CLAIMED;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_release(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_BOUND;
    return GBT_HAL_OK;
}

/* ---- runtime ---- */
gbt_hal_status_t gbt_hal_can_send(gbt_hal_t *h, const gbt_can_frame_t *f)
{
    struct can_frame cf;
    ssize_t n;

    if (h == NULL || gbt_port_linux_to_can(f, &cf) == 0U) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT || h->sock_fd < 0) {
        return GBT_HAL_E_NOT_READY;
    }
    n = write(h->sock_fd, &cf, sizeof cf);
    if (n == (ssize_t)sizeof cf) {
        return GBT_HAL_OK;
    }
    sat_inc(&h->status.tx_error);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)) {
        return GBT_HAL_E_FULL;                         /* not accepted; caller retries */
    }
    if (h->status.state == GBT_CAN_STATE_BUS_OFF) {
        return GBT_HAL_E_BUS_OFF;
    }
    return GBT_HAL_E_IO;                               /* e.g. ENETDOWN */
}

static void handle_error_frame(gbt_hal_t *h, const struct can_frame *cf)
{
    sat_inc(&h->error_frames);
    if ((cf->can_id & CAN_ERR_BUSOFF) != 0U) {
        h->status.state = (uint8_t)GBT_CAN_STATE_BUS_OFF;
    } else if ((cf->can_id & CAN_ERR_RESTARTED) != 0U) {
        h->status.state = (uint8_t)GBT_CAN_STATE_ERROR_ACTIVE;
    } else if ((cf->can_id & CAN_ERR_CRTL) != 0U) {
        if ((cf->data[1] & (CAN_ERR_CRTL_RX_PASSIVE | CAN_ERR_CRTL_TX_PASSIVE)) != 0U) {
            h->status.state = (uint8_t)GBT_CAN_STATE_ERROR_PASSIVE;
        } else if ((cf->data[1] & (CAN_ERR_CRTL_RX_WARNING | CAN_ERR_CRTL_TX_WARNING)) != 0U) {
            h->status.state = (uint8_t)GBT_CAN_STATE_ERROR_WARNING;
        }
    }
}

gbt_hal_status_t gbt_hal_can_receive(gbt_hal_t *h, gbt_can_frame_t *out)
{
    struct can_frame cf;
    struct iovec iov;
    struct msghdr msg;
    union {
        struct cmsghdr align;
        char buf[CMSG_SPACE(sizeof(uint32_t))];
    } ctrl;
    struct cmsghdr *cm;
    ssize_t n;
    unsigned guard;

    if (h == NULL || out == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT || h->sock_fd < 0) {
        return GBT_HAL_E_NOT_READY;
    }
    for (guard = 0U; guard < 8U; guard++) {           /* skip error/invalid frames, bounded */
        memset(&msg, 0, sizeof msg);
        iov.iov_base = &cf;
        iov.iov_len = sizeof cf;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = ctrl.buf;
        msg.msg_controllen = sizeof ctrl.buf;
        n = recvmsg(h->sock_fd, &msg, 0);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return GBT_HAL_E_EMPTY;
            }
            sat_inc(&h->status.rx_invalid);
            return GBT_HAL_E_IO;
        }
        for (cm = CMSG_FIRSTHDR(&msg); cm != NULL; cm = CMSG_NXTHDR(&msg, cm)) {
            if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_RXQ_OVFL) {
                uint32_t drops;
                memcpy(&drops, CMSG_DATA(cm), sizeof drops);
                if (drops != h->kernel_drops_last) {
                    h->status.rx_overflow_hw += (uint32_t)(drops - h->kernel_drops_last);
                    h->kernel_drops_last = drops;
                }
            }
        }
        if (n != (ssize_t)sizeof cf) {
            sat_inc(&h->status.rx_invalid);
            continue;
        }
        if ((cf.can_id & CAN_ERR_FLAG) != 0U) {
            handle_error_frame(h, &cf);
            continue;
        }
        if (gbt_port_linux_from_can(&cf, out) == 0U) {
            sat_inc(&h->status.rx_invalid);
            continue;
        }
        return GBT_HAL_OK;
    }
    return GBT_HAL_E_EMPTY;
}

uint32_t gbt_hal_now_ms(gbt_hal_t *h)
{
    struct timespec ts;
    if (h == NULL || h->lifecycle == GBT_HAL_LC_UNINIT) {
        return 0U;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0U;
    }
    return (uint32_t)((uint64_t)ts.tv_sec * 1000U + (uint64_t)ts.tv_nsec / 1000000U);
}

gbt_hal_status_t gbt_hal_output_apply(gbt_hal_t *h, const gbt_output_cmd_t *cmd)
{
    gbt_hal_status_t st;
    if (h == NULL || cmd == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    if (h->sink.apply == NULL) {
        return GBT_HAL_E_NOT_READY;
    }
    h->in_sink = 1U;
    st = h->sink.apply(h->sink.ctx, cmd);
    h->in_sink = 0U;
    if (st == GBT_HAL_OK) {
        h->accepted_seq = cmd->seq;
    }
    return st;
}

gbt_hal_status_t gbt_hal_output_feedback(gbt_hal_t *h, gbt_output_fb_t *fb)
{
    gbt_hal_status_t st;
    if (h == NULL || fb == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    if (h->sink.feedback == NULL) {
        return GBT_HAL_E_NOT_READY;
    }
    h->in_sink = 1U;
    st = h->sink.feedback(h->sink.ctx, fb);
    h->in_sink = 0U;
    return st;
}

gbt_hal_status_t gbt_hal_can_get_status(gbt_hal_t *h, gbt_can_status_t *s)
{
    if (h == NULL || s == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    *s = h->status;
    return GBT_HAL_OK;
}

void gbt_hal_event(gbt_hal_t *h, uint16_t code, int32_t a, int32_t b)
{
    (void)a;
    (void)b;
    if (h == NULL || h->lifecycle == GBT_HAL_LC_UNINIT) {
        return;
    }
    sat_inc(&h->event_count);
    h->last_event_code = code;
}
