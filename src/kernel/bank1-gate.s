.include "c128.inc"
.include "zeropage.inc"

.export _krnl_bank1_gate, _bank1_gate_size, _krnl_bank1_store, _krnl_bank1_fetch, _krnl_bank1_run
.export _krnl_bank1_capture_context, _krnl_bank1_resume, _cmdosk_context, _bank1_suspend_pending
.import _krnl_service_dispatch

gate = $0E00
target = $0F30
shell_zp = $0F32
app_zp = $0F4C
app_sp = $0F66
app_map = $0F67
request = $0F70
count = request + 4
data = request + 7
bank1 = MMU_CFG_CC65 | $40  ; Same ROM/I/O map, with Bank 1 RAM.

.assert zpspace = 26, error, "Update KRNL_CONTEXT_SIZE for this cc65 zero-page layout"

.bss
_cmdosk_context:
context_sp:     .res 1
context_map:    .res 1
context_zp:     .res zpspace
context_stack:  .res 256
                .res 1 ; Active device, filled in by the session manager.
_bank1_suspend_pending: .res 1
shell_stack:    .res 256
shell_sp:       .res 1
return_a:       .res 1
return_x:       .res 1

.segment "CODE"

_krnl_bank1_store:
        sta target
        stx target+1
        jmp gate+(store-_krnl_bank1_gate)

_krnl_bank1_run:
        sta target
        stx target+1
        jmp gate+(run-_krnl_bank1_gate)

_krnl_bank1_fetch:
        sta target
        stx target+1
        jmp gate+(fetch-_krnl_bank1_gate)

_krnl_bank1_resume:
        jmp gate+(resume-_krnl_bank1_gate)

_krnl_bank1_capture_context:
        php
        sei
        lda app_sp
        sta context_sp
        lda app_map
        sta context_map
        ldx #zpspace-1
capture_zp:
        lda app_zp,x
        sta context_zp,x
        dex
        bpl capture_zp
        ldx app_sp
        inx
capture_stack:
        lda $0100,x
        sta context_stack,x
        inx
        bne capture_stack
        plp
        rts

; Runs under Bank 0 before starting or resuming an application.
save_cpu_stack:
        ldx #0
save_cpu_byte:
        lda $0100,x
        sta shell_stack,x
        inx
        bne save_cpu_byte
        rts

restore_cpu_stack:
        ldx shell_sp
        txs
        inx
restore_cpu_byte:
        lda shell_stack,x
        sta $0100,x
        inx
        bne restore_cpu_byte
        lda return_a
        ldx return_x
        plp
        rts

; Copied to the common sprite reservation before changing the MMU.
_krnl_bank1_gate:
        php
        pha
        txa
        pha
        tya
        pha
        lda MMU_CR
        tax
        and #$40
        bne from_bank1
        pla
        tay
        pla
        tax
        pla
        plp
        jmp _krnl_service_dispatch
from_bank1:
        ; Share hardware stack and zero page, but use the resident C stack
        ; and run ROM interrupts only while Bank 0 is mapped.
        sei
        stx app_map
        tsx
        stx app_sp
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
        jsr _krnl_service_dispatch
        sei
        lda _bank1_suspend_pending
        bne suspended
resume_app:
        ldx #zpspace-1
restore_app:
        lda app_zp,x
        sta c_sp,x
        dex
        bpl restore_app
        lda app_map
        sta MMU_CR
        pla
        tay
        pla
        tax
        pla
        plp
        rts
suspended:
        lda #0
        tax
        jmp gate+(return_shell-_krnl_bank1_gate)

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

fetch:
        ; Reverse of store: read Bank 1 into the common mailbox.
        php
        sei
        lda target
        sta ptr1
        lda target+1
        sta ptr1+1
        lda #bank1
        sta MMU_CR
        ldy #0
fetch_byte:
        lda (ptr1),y
        sta data,y
        iny
        cpy count
        bne fetch_byte
        lda #MMU_CFG_CC65
        sta MMU_CR
        plp
        rts

run:
        php
        sei
        jsr save_cpu_stack
        tsx
        stx shell_sp
        lda #0
        sta _bank1_suspend_pending
        jsr gate+(save_shell_context-_krnl_bank1_gate)
        lda #bank1
        sta MMU_CR
        jsr gate+(entry-_krnl_bank1_gate)
return_shell:
        sta return_a
        stx return_x
        lda #MMU_CFG_CC65
        sta MMU_CR
        ldx #zpspace-1
restore_shell:
        lda shell_zp,x
        sta c_sp,x
        dex
        bpl restore_shell
        jmp restore_cpu_stack

save_shell_context:
        ldx #zpspace-1
save_shell:
        lda c_sp,x
        sta shell_zp,x
        dex
        bpl save_shell
        rts

resume:
        php
        sei
        jsr save_cpu_stack
        tsx
        stx shell_sp
        lda #0
        sta _bank1_suspend_pending
        jsr gate+(save_shell_context-_krnl_bank1_gate)
        ldx #zpspace-1
load_app_zp:
        lda context_zp,x
        sta app_zp,x
        dex
        bpl load_app_zp
        lda context_map
        sta app_map
        ldx context_sp
        stx app_sp
        txs
        inx
load_app_stack:
        lda context_stack,x
        sta $0100,x
        inx
        bne load_app_stack
        jmp gate+(resume_app-_krnl_bank1_gate)
entry:
        jmp (target)

_bank1_gate_size = * - _krnl_bank1_gate
.assert gate + _bank1_gate_size <= target, error, "Bank 1 gate overlaps its state"
.assert shell_zp + zpspace <= app_zp && app_zp + zpspace <= app_sp && app_map < request, error, "Bank 1 gate state overlaps"
