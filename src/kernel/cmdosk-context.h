#ifndef CMDOSK_CONTEXT_H
#define CMDOSK_CONTEXT_H

/* Captured at an app-to-kernel call. The hardware stack contains the
 * saved A/X/Y/P and the return address of the app's call to $0E00. */
#define CMDOSK_CONTEXT_DEVICE (2 + 26 + 256)
#define CMDOSK_CONTEXT_SIZE (CMDOSK_CONTEXT_DEVICE + 1)
extern unsigned char cmdosk_context[CMDOSK_CONTEXT_SIZE];
extern unsigned char bank1_suspend_pending;
void bank1_capture_context(void);
int bank1_resume(void);
unsigned char cmdosk_session_hotkey(unsigned char key);
void cmdosk_session_suspend(void);
void cmdosk_session_menu(void);
void cmdosk_session_switch_pending(void);

#endif
