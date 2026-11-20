.include "cbm_kernal.inc"

.segment "CODE"
        .byte $43, $42, $4D, 0, 0, 0, 0
        .byte $43, $4D, $44, $2D, $4F, $53, 0
        .byte 0

        lda #0
        tax
        jsr SETBNK
        lda #4
        ldx #<filename
        ldy #>filename
        jsr SETNAM
        lda #1
        ldx #8
        ldy #1
        jsr SETLFS
        lda #0
        jsr LOAD
        bcs failed
        jmp $1C01
failed:
        ; Carry reports failure to the C128 autoboot routine.
        rts

filename:
        .byte $42, $4F, $4F, $54
