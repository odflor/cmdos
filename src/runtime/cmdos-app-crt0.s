;
; Startup for a CMD-OS application.
; Unlike the normal C128 startup, this returns to its caller instead of BASIC.
;

        .export         _exit
        .export         __STARTUP__ : absolute = 1
        .import         initlib, donelib
        .import         zerobss
        .import         push0, callmain
        .import         RESTOR, BSOUT, CLRCH
        .import         __MAIN_START__, __MAIN_SIZE__, __STACKSIZE__

        .include        "zeropage.inc"
        .include        "c128.inc"

.segment "STARTUP"

Start:
        ; Select the C128 character set expected by conio.
        lda     #14
        jsr     BSOUT

        ; Use the normal CC65 C128 map while the application runs.
        lda     MMU_CR
        pha
        lda     #MMU_CFG_CC65
        sta     MMU_CR

        ; Preserve the caller's CC65 zero-page state.
        ldx     #zpspace-1
L1:     lda     c_sp,x
        sta     zpsave,x
        dex
        bpl     L1

        jsr     zerobss

        pla
        sta     mmusave
        tsx
        stx     spsave

        lda     #<(__MAIN_START__ + __MAIN_SIZE__ + __STACKSIZE__)
        ldx     #>(__MAIN_START__ + __MAIN_SIZE__ + __STACKSIZE__)
        sta     c_sp
        stx     c_sp+1

        jsr     initlib

        lda     INIT_STATUS
        sta     initsave
        and     #$fe
        sta     INIT_STATUS

        lda     #0
        sta     FNAM_BANK

        jsr     callmain

_exit:
        pha
        jsr     donelib

        ldx     #zpspace-1
L2:     lda     zpsave,x
        sta     c_sp,x
        dex
        bpl     L2

        lda     initsave
        sta     INIT_STATUS

        pla
        sta     STATUS

        ldx     spsave
        txs
        ldx     mmusave
        stx     MMU_CR

        rts

.segment "INIT"
zpsave: .res zpspace

.bss
spsave:   .res 1
mmusave:  .res 1
initsave: .res 1
