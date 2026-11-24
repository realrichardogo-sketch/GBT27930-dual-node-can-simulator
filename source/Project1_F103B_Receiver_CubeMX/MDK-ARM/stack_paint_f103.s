;*******************************************************************************
; stack_paint_f103.s - STEP 07-C2.2-E runtime stack paint (F103, ARMCC V5.06u7)
;
; Purpose
;   Fill the main stack [Stack_Mem, __initial_sp) with 0xA5A5A5A5 once per
;   reset, so that the maximum stack depth can later be measured as
;       used = __initial_sp - (lowest word that is no longer 0xA5A5A5A5)
;   by a debugger scan (no target code is needed for the measurement).
;
; Why here (and not in Reset_Handler)
;   STACK is a ZI section of RW_IRAM1 and is zeroed by __scatterload_zeroinit
;   inside __main. A paint before __main would be erased. This module uses the
;   armlink $Sub$$/$Super$$ mechanism: __rt_entry_main calls |$Sub$$main|
;   (below) instead of main; it paints and then tail-branches to the real main
;   (|$Super$$main|). At that point the C runtime is initialised, no interrupt
;   is enabled (HAL_Init not yet run) and SP == __initial_sp, so the whole
;   stack region is unused.
;
; Boundaries
;   Taken from __user_initial_stackheap (startup_stm32f103xe.s, standard C
;   library build): R1 = Stack_Mem + Stack_Size, R3 = Stack_Mem. No stack size
;   is hard-coded here. The helper only loads R0-R3 and returns (BX LR); it
;   uses no stack. With MicroLIB the helper does not exist and the link fails.
;
; Fail-safe
;   Paint only if SP == R1, R3 < R1 and both bounds are 8-byte aligned;
;   otherwise branch to main without writing memory (a skipped paint is
;   visible: the word at Stack_Mem then reads 0x00000000).
;
; Register use (AAPCS)
;   R4 and LR are saved with PUSH {r4, lr} (8 bytes, keeps 8-byte alignment at
;   the BL) and restored with POP {r4, lr} (Thumb-2 LDMIA.W SP!, {r4, lr}).
;   R0-R3 and the flags are scratch (main is int main(void)). R5-R12 are not
;   used. SP is restored exactly. The paint loop uses no stack and calls
;   nothing. No FRAME directives: armlink lists this function with unknown
;   stack size; its real use is 8 bytes, only during the helper call.
;
; No change to startup, main.c, the scatter layout or any other source.
;*******************************************************************************

STACK_PAINT_WORD    EQU     0xA5A5A5A5

                    PRESERVE8
                    THUMB

                    AREA    |.text|, CODE, READONLY, ALIGN=2

                    IMPORT  __user_initial_stackheap
                    IMPORT  |$Super$$main|
                    EXPORT  |$Sub$$main|

|$Sub$$main|       PROC
                    PUSH    {r4, lr}
                    BL      __user_initial_stackheap   ; R0 = Heap_Mem, R1 = Stack_Mem + Stack_Size
                                                       ; R2 = Heap_Mem + Heap_Size, R3 = Stack_Mem
                    POP     {r4, lr}                   ; SP back to its entry value

                    ; ---- preconditions (any failure: no memory is written) ----
                    MOV     r2, sp
                    CMP     r2, r1                     ; SP must equal the stack top
                    BNE     sp_skip
                    CMP     r3, r1                     ; stack base must be below the top
                    BHS     sp_skip
                    ORR     r2, r1, r3
                    TST     r2, #7                     ; both bounds 8-byte aligned
                    BNE     sp_skip

                    ; ---- paint [R3, R1) with word stores; no calls, no stack ----
                    LDR     r2, =STACK_PAINT_WORD
sp_loop             STR     r2, [r3], #4
                    CMP     r3, r1
                    BLO     sp_loop

sp_skip             B       |$Super$$main|             ; tail branch; LR returns to __rt_entry_main
                    ENDP

                    ALIGN
                    END
