# STEP 07-C1.1 — SPSC CAN RX Queue Design (G-SPSC)

| Field | Value |
|---|---|
| Date | 2026-11-16 |
| Files | `ports/common/gbt_spsc.h`, `ports/common/gbt_spsc.c` |
| Status | Implemented, host-tested, GCC Cortex-M3/M4F cross-compiled. **ARMCC V5.06 not compiled.** Not yet used by any port (C1.2/C1.3) |
| Architecture | `docs/ARCHITECTURE.md` §4.1 steps 1–3, §5 "Port RX queue (ISR -> main, SPSC; gated by G-SPSC)" |

## 1. Purpose and placement

The CAN RX interrupt of each STM32 port (producer) copies received frames into
this queue; the main loop (consumer) takes them out through the port's
`gbt_hal_can_receive()`. The queue lives in `ports/common/` because both STM32
ports share it. It includes only `<stdint.h>`, `<stddef.h>` and
`common/gbt_types.h` (for `gbt_can_frame_t`): no HAL, no protocol, no RTOS, no
compiler intrinsics. `protocol/gbt_ring` is **not** used: it is single-context
(both push and pop modify `count`).

## 2. API

| Function | Context | Result |
|---|---|---|
| `gbt_spsc_init(q)` | before the producer interrupt is enabled | empty queue, `dropped = 0`; `E_PARAM` on NULL |
| `gbt_spsc_push(q, f)` | producer (ISR) only | `OK` stored; `E_FULL` not stored and `dropped` +1 (saturating); `E_PARAM` on NULL |
| `gbt_spsc_pop(q, out)` | consumer (main) only | `OK` oldest frame copied; `E_EMPTY` (`*out` untouched); `E_PARAM` |
| `gbt_spsc_count(q)` | consumer only | frames waiting, 0..16 (lower bound; may grow concurrently) |
| `gbt_spsc_dropped(q)` | any | cumulative drops (saturating at 0xFFFFFFFF) |

There is no reset function (requirement: no reset while interrupts are active).
The queue does not validate frame content; frame validation and the
`rx_invalid` counter stay in the port.

## 3. Memory and capacity

```
typedef struct {
    volatile gbt_can_frame_t slot[16];   /* 16 x 16 bytes (14 + 2 padding)  offset 0x000 */
    volatile uint32_t        head;       /* producer-owned                  offset 0x100 */
    volatile uint32_t        tail;       /* consumer-owned                  offset 0x104 */
    volatile uint32_t        dropped;    /* producer-owned                  offset 0x108 */
} gbt_spsc_t;                            /* 268 bytes on arm-none-eabi (offsets from disassembly) */
```

- Capacity `GBT_SPSC_CAPACITY` = 16 frames; **all 16 slots are usable** (no
  sacrificial slot).
- `head` and `tail` are free-running counters (wrap at 2^32). Fill level =
  `head - tail` (unsigned), always 0..16. Slot index = counter & 15. Because 16
  divides 2^32, the index sequence is continuous across the counter wrap.
- Empty: `head == tail`. Full: `head - tail >= 16`.
- Static storage only (caller-allocated object); no allocation anywhere.
- Code size (arm-none-eabi-gcc 10.3.1, Cortex-M3, -Os): 256 bytes `.text`, no
  `.data`/`.bss`.

## 4. Ownership

| Object | Written by | Read by |
|---|---|---|
| `head` | producer only (publication) | producer, consumer |
| `tail` | consumer only (release) | consumer, producer |
| `dropped` | producer only (read-modify-write in one context) | anyone |
| `slot[head & 15]` while free (`head - tail < 16`) | producer only | — |
| `slot[tail & 15]` while published (`head != tail`) | — | consumer only |

No variable has two writers, so no atomic read-modify-write is needed; the
only concurrent operations are a single-writer store racing a reader's load of
the same aligned 32-bit word.

## 5. Concurrency assumptions (must hold on every target)

| # | Assumption | Holds for G474 / F103 because |
|---|---|---|
| A1 | Exactly one producer context per queue | Each port pushes only from its CAN RX interrupt (G474 `FDCAN1_IT0`; F103 `USB_LP_CAN1_RX0`). If more than one callback pushes (e.g. new-message and error callback), they run inside the same IRQ handler, so they never preempt each other. Two different IRQ lines must not push into the same queue unless they have the same preemption priority (C1.2/C1.3 review item). |
| A2 | Exactly one consumer context | Only the main loop (through `gbt_hal_can_receive`) pops; no other ISR pops. |
| A3 | Single core; producer and consumer share the same memory view; no data cache; no DMA writes the queue | Cortex-M3 (F103) and Cortex-M4 (G474) are single-core with no data cache (the G4 ART accelerator caches flash instructions/constants, not SRAM data). |
| A4 | Aligned 32-bit loads/stores of `head`/`tail` are single accesses | ARMv7-M: word-aligned `LDR`/`STR` are single-copy atomic. The struct places the indices at word-aligned offsets. Verified in GCC disassembly (§6); ARMCC listing to be checked in C1.4. |
| A5 | `gbt_spsc_init` runs before the producer interrupt is enabled | Port init order (C1.2/C1.3): init queue, then enable the CAN notifications. |

## 6. Memory ordering

**Requirement.** In `push`, all slot stores must be performed before the
`head` store that publishes the slot. In `pop`, all slot loads must be
performed before the `tail` store that releases the slot. Each index load must
be one access, not split or repeated.

**Compiler ordering (portable C99).** Every access to shared state — the slot
fields, `head`, `tail` and `dropped` — is made through a volatile-qualified
lvalue. In C99 (5.1.2.3) accessing a volatile object is a side effect, and
side effects must be complete at each sequence point in abstract-machine
order, so a conforming compiler (GCC, arm-none-eabi-gcc, ARMCC V5 `--c99`) may
not move one volatile access across another. The index publication is the last
statement of `push` and `pop`. Making only the indices volatile would *not* be
enough, because then the compiler could move the non-volatile slot accesses
across the index store — which is why the slots are volatile as well. This is
the only property of `volatile` relied upon; `volatile` is **not** relied on
for atomicity (A4 covers that) nor for inter-processor ordering (A3 covers
that).

**CPU ordering.** On a single core the interrupt and the interrupted thread are
the same observer. An exception is taken between instructions (Cortex-M3/M4
can abandon and restart multi-register `LDM/STM`, but the queue uses single
word and byte accesses), and a processor observes its own memory accesses in
program order, so no hardware barrier (DMB) is needed under A3. ARM's guidance
for Cortex-M (application note AN321 on memory barrier instructions) is the
relevant reference; it was not re-read in this session and is cited for the
Owner's confirmation, not as evidence.

**Platform hook.** `gbt_spsc.c` contains `GBT_SPSC_HW_BARRIER()` immediately
before each index publication. It is empty unless the build defines it. It
exists for a future target that breaks A3 (multi-core, data cache, DMA); such
a port must define it as a data memory barrier on the compiler command line.
No architecture-specific instruction is in the shared code.

**Evidence (arm-none-eabi-gcc 10.3.1, Cortex-M3 and Cortex-M4F, -Os and -O2;
`docs/evidence/step07c1_1/gbt_spsc_*.dis`).**
- `push`: `ldr.w [q,#0x100]` (head) and `ldr.w [q,#0x104]` (tail) — one word load each; then the slot stores (`str` id, `strb` dlc, flags, 8 data bytes); then `str.w [q,#0x100]` (head) as the last memory access on the success path.
- `pop`: `ldr.w [q,#0x104]` (tail), `ldr.w [q,#0x100]` (head); slot loads; `str.w [q,#0x104]` (tail) last.
- `dropped`: one `ldr.w`/`str.w` pair at `0x108` on the full path only.

ARMCC V5.06 output has not been generated (Keil is not available in WSL).
C1.4 must inspect the ARMCC listing (`--asm` / `.txt` listing) for the same
three properties.

## 7. Overflow behaviour

- Full queue: the new frame is rejected (the oldest frames are kept, the same
  policy as the G474 FIFO0 "blocking" mode, ND-9), `push` returns `E_FULL`, and
  `dropped` is incremented (saturating).
- The ISR must still remove the frame from the hardware FIFO when `push`
  returns `E_FULL`, so hardware-FIFO overflow (`rx_overflow_hw`) and queue
  overflow (`rx_overflow_sw`) stay distinguishable.
- The port reports `gbt_spsc_dropped()` as `rx_overflow_sw` in
  `gbt_hal_can_get_status()`; `app/gbt_integration` already forwards any
  increase to `gbt_node_on_rx_overflow()` (T-5 behaviour, tested in SIL).

## 8. Sizing note

At 250 kbit/s an extended data frame with 8 bytes takes about 0.5 ms on the
bus, so at most about 2 frames arrive per 1 ms main-loop period, against a
queue of 16 and a per-step drain of 16 (`GBT_INTEG_RX_PER_STEP`). A loop stall
longer than about 8 ms would be needed to fill the queue; such a stall would
already violate the 1 ms loop budget of the 10 ms safety requirement.
