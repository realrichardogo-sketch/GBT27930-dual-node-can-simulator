/* protocol/gbt_codec_internal.h - helpers shared by protocol codec modules only (not public API). */
#ifndef GBT_CODEC_INTERNAL_H
#define GBT_CODEC_INTERNAL_H

#include <stdint.h>
#include "common/gbt_types.h"
#include "protocol/gbt_codec.h"
#include "protocol/gbt_id.h"

/* Little-endian byte helpers (§5.3 low byte first). */
void     gbt_codec_put_u16(uint8_t *p, uint16_t v);
void     gbt_codec_put_u24(uint8_t *p, uint32_t v);
void     gbt_codec_put_u32(uint8_t *p, uint32_t v);
uint16_t gbt_codec_get_u16(const uint8_t *p);
uint32_t gbt_codec_get_u24(const uint8_t *p);
uint32_t gbt_codec_get_u32(const uint8_t *p);

/* Start an extended frame for msg with DLC = len; data zeroed. */
gbt_codec_status_t gbt_codec_frame_begin(gbt_msg_t msg, uint8_t len, gbt_can_frame_t *f);
/* Check extended format, identifier of msg and DLC == len. */
gbt_codec_status_t gbt_codec_frame_check(const gbt_can_frame_t *f, gbt_msg_t msg, uint8_t len);

#endif /* GBT_CODEC_INTERNAL_H */
