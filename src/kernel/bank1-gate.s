.include "c128.inc"
.include "zeropage.inc"

.export _bank1_gate, _bank1_gate_size, _bank1_store, _bank1_run
.import _tui_dispatch

gate = $0E00
target = $0F30
shell_zp = $0F32
app_zp = $0F4C
request = $0F70
count = request + 4
data = request + 7
bank1 = MMU_CFG_CC65 | $40  ; Same ROM/I/O map, with Bank 1 RAM.

.segment "CODE"

_bank1_store:
        sta target
        stx target+1
        jmp gate+(store-_bank1_gate)

_bank1_run:
        sta target
        stx target+1
        jmp gate+(run-_bank1_gate)

; Copied to the common sprite reservation before changing the MMU.
_bank1_gate:
        ; Share hardware stack and zero page, but use the resident C stack
        ; and run ROM interrupts only while Bank 0 is mapped.
        sei
        ldx #zpspace-1
save_app:
        lda c_sp,x
        sta app_zp,x
        lda shell_zp,x
        sta c_sp,x
        dex
        bpl save_app
        lda #MMU_CFG_CC65
        sta MMU_CR
        cli
        jsr _tui_dispatch
        sei
        ldx #zpspace-1
restore_app:
        lda app_zp,x
        sta c_sp,x
        dex
        bpl restore_app
        lda #bank1
        sta MMU_CR
        rts

store:
        ; The caller supplies a nonzero chunk fitting the common mailbox.
        php
        sei
        lda target
        sta ptr1
        lda target+1
        sta ptr1+1
        lda #bank1
        sta MMU_CR
        ldy #0
copy:
        lda data,y
        sta (ptr1),y
        iny
        cpy count
        bne copy
        lda #MMU_CFG_CC65
        sta MMU_CR
        plp
        rts

run:
        php
        sei
        ldx #zpspace-1
save_shell:
        lda c_sp,x
        sta shell_zp,x
        dex
        bpl save_shell
        lda #bank1
        sta MMU_CR
        jsr gate+(entry-_bank1_gate)
        lda #MMU_CFG_CC65
        sta MMU_CR
        ldx #zpspace-1
restore_shell:
        lda shell_zp,x
        sta c_sp,x
        dex
        bpl restore_shell
        plp
        rts
entry:
        jmp (target)

_bank1_gate_size = * - _bank1_gate
.assert gate + _bank1_gate_size <= target, error, "Bank 1 gate overlaps its state"
.assert shell_zp + zpspace <= app_zp && app_zp + zpspace <= request, error, "Bank 1 zero-page copies overlap"
