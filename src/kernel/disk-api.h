#ifndef CMDOS_DISK_API_H
#define CMDOS_DISK_API_H

unsigned char krnl_file_open(unsigned char device, const char* name,
                             unsigned char type, unsigned char write,
                             unsigned char* result);
int krnl_file_transfer(unsigned char handle, void* data, unsigned char count,
                       unsigned char write);
unsigned char krnl_file_close(unsigned char handle, unsigned char abort);

#endif
