.export _krnl_vdc_vram_kib

.bss
original_low: .res 1
original_high: .res 1
marker_readback: .res 1

.code
_krnl_vdc_vram_kib:
        php
        sei
        ldx #18
        jsr read_register
        pha
        ldx #19
        jsr read_register
        pha
        ldx #28
        jsr read_register
        pha
        ora #$10
        jsr write_register

        ; Follow examples/vdc test.prg: write $AA at $0000 and $55 at $4000.
        ldy #$00
        jsr address
        jsr read_register
        sta original_low
        ldy #$40
        jsr address
        jsr read_register
        sta original_high
        ldy #$00
        jsr address
        lda #$aa
        jsr write_register
        ldy #$40
        jsr address
        lda #$55
        jsr write_register
        ldy #$00
        jsr address
        jsr read_register
        cmp #$aa
        bne vram16

        ; Some VDC emulations remap the missing DRAM address line to A15.
        ; Repeat the BASIC test with $8000 before confirming 64 KB.
        ldy #$80
        jsr address
        jsr read_register
        pha
        ldy #$80
        jsr address
        lda #$55
        jsr write_register
        ldy #$00
        jsr address
        jsr read_register
        sta marker_readback
        ldy #$80
        jsr address
        pla
        jsr write_register
        lda marker_readback
        cmp #$aa
        bne vram16
        lda #64
        bne restore
vram16:
        lda #16
restore:
        pha
        ldy #$40
        jsr address
        lda original_high
        jsr write_register
        ldy #$00
        jsr address
        lda original_low
        jsr write_register
        pla
        tay
        pla
        ldx #28
        jsr write_register
        pla
        ldx #19
        jsr write_register
        pla
        ldx #18
        jsr write_register
        tya
        ldx #0
        plp
        rts

address:
        ldx #18
        tya
        jsr write_register
        inx
        lda #0
        jsr write_register
        ldx #31
        rts

read_register:
        jsr select_register
        lda $d601
        rts

write_register:
        jsr select_register
        sta $d601
        rts

select_register:
        stx $d600
wait:
        bit $d600
        bpl wait
        rts
