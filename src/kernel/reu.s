.export _krnl_reu_probe
.export _krnl_reu_mount, _krnl_reu_format
.export _krnl_reu_dir_read, _reu_dir_name
.export _reu_dir_size, _reu_dir_type
.export _krnl_reu_transfer, _reu_offset, _reu_buffer, _reu_length
.export _reu_position
.export _krnl_reu_write_entry

.bss
saved: .res 9
sample: .res 8
_reu_dir_name: .res 19
_reu_dir_size: .res 2
_reu_dir_type: .res 1
low: .res 1
high: .res 2
probe_bank: .res 1
_reu_buffer: .res 2
_reu_position:
_reu_offset: .res 3
_reu_length: .res 2

.code
; Capacity in 64 KiB banks. $FF represents 256 banks (16 MiB).
_krnl_reu_probe:
        php
        sei
        lda $df04
        pha
        eor #$ff
        sta $df04
        cmp $df04
        beq present
        pla
        sta $df04
        lda #0
        plp
        rts
present:
        pla
        sta $df04
        ldx #8
save:   lda $df02,x
        sta saved,x
        dex
        bpl save
        lda #0
        sta $df09
        sta $df0a
        tax
        lda #$91
        jsr dma
        lda sample
        sta low
        lda #1
        sta probe_bank
probe_next:
        ldx probe_bank
        lda #$91
        jsr dma
        lda sample
        sta high
        lda sample+1
        sta high+1
        lda low
        sta sample+1
        eor #$ff
        sta sample
        lda #$90
        jsr dma
        ; Check for aliasing with bank zero. Require two distinct bytes
        ; in the candidate bank to reject the 1764's floating bus.
        ldx #0
        lda #$91
        jsr dma
        lda sample
        cmp low
        bne restore_byte
        ldx probe_bank
        lda #$91
        jsr dma
        lda low
        eor #$ff
        cmp sample
        bne restore_byte
        lda low
        cmp sample+1
restore_byte:
        php
        ldx probe_bank
        lda high
        sta sample
        lda high+1
        sta sample+1
        lda #$90
        jsr dma
        plp
        bne restore_registers
        asl probe_bank
        bne probe_next
        dec probe_bank
restore_registers:
        ldx #8
restore:
        lda saved,x
        sta $df02,x
        dex
        bpl restore
        lda probe_bank
        plp
        rts

dma:
        ldy #<sample
        sty $df02
        ldy #>sample
        sty $df03
        ldy #0
        sty $df04
        sty $df05
        stx $df06
        sty $df08
        ldy #2
        sty $df07
        sta $df01
        rts

; Header v1 at REU offset zero. Reserved bytes must be zero.
header: .byte $43,$4d,$4f,$53,$02,$00,$00,$00

_krnl_reu_mount:
        lda #$91
        jsr header_dma
        ldx #7
compare:
        lda sample,x
        cmp header,x
        bne invalid
        dex
        bpl compare
        lda #1
        rts
invalid:
        lda #0
        rts

_krnl_reu_format:
        ; Sixteen 32-byte records at $000100. Zero first byte means free.
        lda #0
        sta _reu_dir_name
        sta $df07
        lda #2
        sta $df08
        lda #$80
        sta $df0a
        ldx #0
        lda #$90
        ldy #1
        jsr directory_dma
        ldx #7
initialize:
        lda header,x
        sta sample,x
        dex
        bpl initialize
        lda #$90
        jmp header_dma

header_dma:
        pha
        ldx #6
header_setup:
        lda header_transfer,x
        sta _reu_buffer,x
        dex
        bpl header_setup
        pla
        jmp _krnl_reu_transfer

header_transfer: .word sample
        .byte 0,0,0
        .word 8

; Fastcall slot 0..15. Fetch the name and little-endian byte count.
_krnl_reu_dir_read:
        jsr directory_address
        lda #19
        sta $df07
        lda #0
        sta $df08
        sta $df0a
        lda #$91
        jsr directory_dma
        lda _reu_dir_name+18
        sta _reu_dir_type
        lda _reu_dir_name+17
        sta _reu_dir_size+1
        lda _reu_dir_name+16
        sta _reu_dir_size
        lda #0
        sta _reu_dir_name+16
        rts

_krnl_reu_write_entry:
        jsr directory_address
        stx _reu_offset
        sty _reu_offset+1
        lda #0
        sta _reu_offset+2
        lda #$90
        jmp _krnl_reu_transfer

directory_address:
        ldy #1
        asl
        asl
        asl
        asl
        asl
        tax
        bcc dir_read_page
        iny
dir_read_page:
        rts
directory_dma:
        stx $df04
        sty $df05
        ldx #<_reu_dir_name
        stx $df02
        ldx #>_reu_dir_name
        stx $df03
        ldx #0
        stx $df06
        stx $df09
        sta $df01
        rts

; Fastcall command: $90 writes, $91 reads. A zero length is a no-op.
; Load the seven contiguous REU registers from the transfer parameters.
_krnl_reu_transfer:
        ldx _reu_length
        bne transfer
        ldx _reu_length+1
        beq no_transfer
transfer:
        pha
        ldx #6
load_register:
        lda _reu_buffer,x
        sta $df02,x
        dex
        bpl load_register
        ldx #0
        stx $df09
        stx $df0a
        pla
        sta $df01
no_transfer:
        rts
