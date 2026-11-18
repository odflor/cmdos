.include "zeropage.inc"
.export __STARTUP__ : absolute = 1
.import zerobss, _main

.segment "STARTUP"
        lda #<$3800
        sta c_sp
        lda #>$3800
        sta c_sp+1
        jsr zerobss
        jmp _main
