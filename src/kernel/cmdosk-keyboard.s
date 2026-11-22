.export _cmdosk_bind_help

; PFKEY takes the address of a three-byte pointer in zero page.
PFKEY = $ff65
key_pointer = $fa

.rodata
help_key: .byte $84

.code
_cmdosk_bind_help:
        lda #<help_key
        sta key_pointer
        lda #>help_key
        sta key_pointer+1
        lda #0
        sta key_pointer+2
        lda #key_pointer
        ldx #10
        ldy #1
        jmp PFKEY
