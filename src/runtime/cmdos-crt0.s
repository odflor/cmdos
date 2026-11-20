.include "c128.inc"
.include "zeropage.inc"
.include "cbm_kernal.inc"

.export __STARTUP__ : absolute = 1
.export _exit
.import zerobss, initlib, callmain

.segment "LOADADDR"
        .word $1C01

.segment "STARTUP"
        sei
        cld
        ldx #$FF
        txs

        ; RAM under BASIC, with the KERNAL, screen editor and I/O visible.
        lda #MMU_CFG_CC65
        sta MMU_CR
        lda #<$C000
        sta c_sp
        lda #>$C000
        sta c_sp+1
        lda INIT_STATUS
        and #$FE
        sta INIT_STATUS
        lda #0
        tax
        jsr SETBNK
        cli

        jsr zerobss
        jsr initlib
        lda #14
        jsr BSOUT
        jsr callmain

_exit:
        jmp _exit
