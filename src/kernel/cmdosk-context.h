#ifndef CMDOSK_CONTEXT_H
#define CMDOSK_CONTEXT_H

/* Captured at an app-to-kernel call. The hardware stack contains the
 * saved A/X/Y/P and the return address of the app's call to $0E00. */
#define KRNL_CONTEXT_DEVICE (2 + 26 + 256)
#define KRNL_CONTEXT_SIZE (KRNL_CONTEXT_DEVICE + 1)
extern unsigned char cmdosk_context[KRNL_CONTEXT_SIZE];
extern unsigned char bank1_suspend_pending;
void krnl_bank1_capture_context(void);
int krnl_bank1_resume(void);
unsigned char krnl_session_hotkey(unsigned char key);
void krnl_session_suspend(void);
void krnl_session_menu(void);
void krnl_session_switch_pending(void);

void krnl_session_boot_notice(void);
unsigned char krnl_session_confirm_exit(void);

#endif
