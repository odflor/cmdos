#ifndef CMDOS_DISK_API_H
#define CMDOS_DISK_API_H

#include <cbm.h>

/* TUI_DISK_API returns this table for native Bank 0 CMDPs. Result bit 6
 * signals API availability, bit 0 signals a mounted M: volume.
 * Pointers are valid until CMDP exit. */
struct disk_api {
    unsigned char (__fastcall__ *open)(unsigned char, unsigned char, unsigned char, const char*);
    void (__fastcall__ *close)(unsigned char);
    int (__fastcall__ *read)(unsigned char, void*, unsigned int);
    int (__fastcall__ *write)(unsigned char, const void*, unsigned int);
    unsigned char (*opendir)(unsigned char, unsigned char, ...);
    unsigned char (__fastcall__ *readdir)(unsigned char, struct cbm_dirent*);
    unsigned char (*iec_status)(void);
    unsigned char (*dos_status)(void);
    void (__fastcall__ *read_entry)(unsigned char);
    void (__fastcall__ *write_entry)(unsigned char);
    void (__fastcall__ *transfer)(unsigned char);
    char* name;
    unsigned int* size;
    unsigned char* type;
    void** buffer;
    unsigned int* length;
    unsigned int* position;
    unsigned char* offset;
};

#endif
