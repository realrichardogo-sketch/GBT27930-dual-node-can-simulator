# HARDWARE_TOPOLOGY — STEP 07-C2.4 (frozen)

Two active software CAN nodes. Nothing in this package changes the topology.

| | Node 1 | Transceiver carrier | Node 2 |
|---|---|---|---|
| Board | NUCLEO-G474RE | F103 A development board | F103 B development board (STM32F103ZET6) |
| Role | **Charger**, SA 0x56, TP **receiver** | **Passive** — not a node | **Vehicle**, SA 0xF4, TP **sender** |
| CAN peripheral | FDCAN1, Classical CAN | its MCU must not drive TXD/RXD and must not run a CAN application | bxCAN1 |
| TX / RX pins | PA12 → PD1050S TXD, PA11 ← PD1050S RXD (wired to the F103 A transceiver) | PD1050S | PB9 CAN_TX, PB8 CAN_RX (AFIO remap 2) → onboard PD1050S |
| Firmware | `source/Project1_G474` | none active (see §3) | `source/Project1_F103B_Receiver_CubeMX` |

Bus: CANH–CANH, CANL–CANL, common GND, two 120 Ω terminations (≈ 60 Ω between CANH and CANL,
power off). Classical CAN 2.0B, 29-bit identifiers only, 250 kbit/s, no CAN FD.

## 1. Verified firmware configuration (source evidence)

### G474 (charger)
| Item | Value | Source |
|---|---|---|
| Frame format / mode | `FDCAN_FRAME_CLASSIC`, `FDCAN_MODE_NORMAL`, automatic retransmission ENABLE | `Core/Src/fdcan.c:42-44` |
| Kernel clock | PCLK1 = 170 MHz (HSE 24 MHz /6 ×85 /2; APB1 /1) | `fdcan.c:82`, `main.c` SystemClock_Config, `stm32g4xx_hal_conf.h:118` |
| Bit timing | prescaler 40, SEG1 14, SEG2 2, SJW 2 → 170 MHz / 40 / 17 = **250 kbit/s**, sample point 15/17 = **88.2 %** | `fdcan.c:47-50` |
| Pins | PA11 + PA12, `GPIO_MODE_AF_PP`, `GPIO_AF9_FDCAN1` | `fdcan.c:96-100` |
| Filters | 1 extended filter: mask 0x0000FFFF, ID (0x56 << 8) \| 0xF4 → RX FIFO 0; global filter rejects every other frame and every remote frame | `ports/stm32/g474/port_g474.c` `gbt_port_g474_init` |
| Interrupt | `FDCAN1_IT0_IRQn` (RX FIFO 0 new / lost, bus-off, error passive, warning); TX interrupts not used | `fdcan.c:104-105`, `port_g474.c` |
| TX confirmation | TX FIFO request → `TXBTO` bit observed (STEP 07-C2.4, main context) | `port_g474.c` `tx_poll` |

### F103 B (vehicle)
| Item | Value | Source |
|---|---|---|
| Mode | `CAN_MODE_NORMAL`, AutoRetransmission ENABLE, AutoBusOff DISABLE (bus-off is not left automatically) | `Core/Src/can.c:42-49` |
| Kernel clock | APB1 = 36 MHz (HSE 8 MHz ×9 = 72 MHz, APB1 /2) | `main.c` SystemClock_Config, `stm32f1xx_hal_conf.h:87` |
| Bit timing | prescaler 9, BS1 13, BS2 2, SJW 1 → 36 MHz / 9 / 16 = **250 kbit/s**, sample point 14/16 = **87.5 %** | `can.c:41-45`, `.ioc` CAN.CalculateBaudRate=250000 |
| PB8 | `GPIO_MODE_INPUT`, no pull (CAN_RX) | `can.c:79-82` |
| PB9 | `GPIO_MODE_AF_PP`, high speed (CAN_TX) | `can.c:84-87` |
| Remap | `__HAL_RCC_AFIO_CLK_ENABLE(); __HAL_AFIO_REMAP_CAN1_2();` → `AFIO_MAPR.CAN_REMAP = 10` (CANRX PB8, CANTX PB9). The HAL macro writes SWJ_CFG = 111b, which RM0008 defines as "no effect" (the debug port stays as configured) | `can.c:89-90`, `stm32f1xx_hal_gpio_ex.h:298, 840-845`, `stm32f103xe.h:2566-2568` |
| Filter | bank 0, 32-bit ID/mask, IDE = 1, RTR = 0, ID bits 15..0 = (0xF4 << 8) \| 0x56 → FIFO 0 | `port_f103.c` `gbt_port_f103_init` |
| Interrupt | `USB_LP_CAN1_RX0_IRQn` (FIFO 0 pending / overrun); SCE (error) interrupt not enabled: state polled from ESR | `can.c:93-94`, `port_f103.c` |
| TX confirmation | one mailbox in flight; completion = `TSR.RQCPx` with `TXOKx` | `port_f103.c` `tx_poll` |

Both nodes sample at 87.5–88.2 %: compatible for 250 kbit/s.

Observation (no change made): `can.c` configures PB9 as AF push-pull (line 84-87) a few
instructions **before** the remap (line 90). During that window PB9 is an AF output without the
CAN routing. This happens once per reset, before CAN leaves initialisation mode; it is
CubeMX-generated code and was left unchanged.

## 2. PD1050S interface — what the firmware does and does not do

* The F103 B firmware configures **only PB8/PB9** for CAN (`gpio.c` enables the GPIOA/GPIOB clocks
  and configures no other pin, `gpio.c:42-49`). There is **no** transceiver standby / silent
  control pin in either project.
* If the PD1050S mode pin (pin 8 on TJA1050-class parts, "S" / "STB") of either board is wired to
  an MCU GPIO instead of GND, that pin is a floating input after reset and the transceiver mode
  depends on the part's internal bias. **Owner check (schematic):** pin 8 of both PD1050S must be
  tied to GND (normal mode) or driven low. If F103 B routes it to a GPIO, tell me the pin and I will
  add a USER CODE GPIO initialisation; it is not added without evidence.
* Logic levels (owner check with the datasheets): the PD1050S supply (5 V on most carrier boards)
  determines its RXD high level. The G474 PA11 and F103 PB8 inputs must tolerate it (STM32 "FT"
  pins), and the PD1050S TXD input must accept a 3.3 V high level.

## 3. F103 A — passive carrier requirements

The G474 drives the F103 A transceiver's TXD and reads its RXD. F103 A's own MCU is connected to
the same TXD/RXD nets on a typical carrier board. If that MCU runs a program that configures its
CAN pins (AF push-pull TX), **two outputs drive the same TXD net** (contention) and F103 A could
also acknowledge frames as a third controller. Required before every test:

1. F103 A's MCU is **held in reset** (NRST to GND, e.g. jumper or held RESET button) for the whole
   run — reset state = all GPIOs floating inputs — **or** its flash contains no program that touches
   the CAN pins (owner-verified; this package does not flash F103 A).
2. The transceiver is still powered (the carrier's 5 V / 3.3 V rails are on).
3. No board is back-powered through the TXD/RXD wires (check with the G474 unpowered: F103 A rails
   must not rise from the signal lines, and vice versa).

## 4. CAN identifiers on the bus (from the implementation, `protocol/gbt_id.c`, `protocol/gbt_tp.c`)

`ID = priority << 26 | PF << 16 | PS(destination) << 8 | SA`.

| Frame | Direction | ID |
|---|---|---|
| TP.CM RTS / abort (vehicle) | F103 B → G474 | `0x1CEC56F4` |
| TP.CM CTS / EOM_ACK / abort (charger) | G474 → F103 B | `0x1CECF456` |
| TP.DT | F103 B → G474 | `0x1CEB56F4` |
| CHM / CRM / CEM (examples) | G474 → F103 B | `0x1826F456` / `0x1801F456` / `0x081FF456` |
| BHM / BEM (examples) | F103 B → G474 | `0x182756F4` / `0x081E56F4` |

Every vehicle frame matches the G474 filter (bits 15..0 = 0x56F4); every charger frame matches the
F103 B filter (bits 15..0 = 0xF456).
