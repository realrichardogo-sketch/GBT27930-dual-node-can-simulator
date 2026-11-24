/*
 * app/output_if.h - application output BACKEND interface (SAF-4: outside protocol/).
 * Status: STEP 07-A.1 interface proposal (pending Owner architecture review).
 *
 * Role: an application-level adapter that IMPLEMENTS the ND-1 HAL output sink.
 * It does not replace the sink contract and it is not a second output path:
 *
 *   gbt_node_get_output -> integration -> gbt_hal_output_apply -> sink.apply
 *                                                                   = output_if backend
 *
 * Application code never calls the backend's apply directly; the single
 * authoritative path to the physical output is gbt_hal_output_apply().
 *
 * Backends per target:
 *   Linux SIL / tests : simulated plant (records commands and acceptance time)
 *   STM32 Project 1   : stores the command (no power stage)
 *   Project 2         : power-converter backend (AC-15: backend selection only)
 *
 * The protocol core never includes this header.
 */
#ifndef GBT_OUTPUT_IF_H
#define GBT_OUTPUT_IF_H

#include "hal/hal.h"   /* gbt_output_sink_t, gbt_output_cmd_t, gbt_output_fb_t */

/* Backend instance; complete type defined by the selected backend. */
typedef struct output_if_backend output_if_backend_t;

/*
 * Fill *sink with the backend's apply/feedback callbacks and ctx = backend.
 * The caller then binds it with gbt_hal_bind_output_sink(). The backend
 * object must outlive the binding (until gbt_hal_unbind_output_sink returns).
 * Callback rules are those of hal/hal.h (main context, bounded, no re-entry).
 * Returns GBT_HAL_E_PARAM if an argument is NULL, else GBT_HAL_OK.
 */
gbt_hal_status_t output_if_make_sink(output_if_backend_t *backend, gbt_output_sink_t *sink);

#endif /* GBT_OUTPUT_IF_H */
