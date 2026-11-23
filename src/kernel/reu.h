#ifndef CMDOS_REU_H
#define CMDOS_REU_H

extern char reu_dir_name[19];
extern unsigned char reu_dir_type, reu_offset[3];
extern unsigned int reu_position;
extern unsigned int reu_dir_size;
extern unsigned int reu_length;
extern void* reu_buffer;
void __fastcall__ krnl_reu_dir_read(unsigned char slot);
void __fastcall__ krnl_reu_write_entry(unsigned char slot);
void __fastcall__ krnl_reu_transfer(unsigned char command);
unsigned char krnl_reu_find(const char* name);
unsigned int krnl_reu_read(void* buffer, unsigned int size);
void krnl_reu_publish(unsigned char slot, const char* name,
                       unsigned int size, unsigned char type);

#endif
