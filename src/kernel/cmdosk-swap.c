#include <string.h>
#include "cmdosk-abi.h"
#include "cmdosk-swap.h"
#include "filename.h"
#include "reu.h"

#define BANK1_START 0x1000u
#define BANK1_END 0xC000u
#define BANK1_SIZE (BANK1_END - BANK1_START)
#define HEADER_SIZE 9
#define REU_STATUS (*(volatile unsigned char*)0xDF00)

/* KSWP identifies snapshots tied to the kernel ABI. */
static const unsigned char signature[] = {0x4B, 0x53, 0x57, 0x50, KRNL_ABI_VERSION};

void __fastcall__ krnl_bank1_fetch(unsigned int address);

static unsigned char swap_name(const char* name)
{
    const char* suffix = strchr(name, '.');
    return suffix && !strcmp(suffix, ".swp") && krnl_valid_filename(name);
}

static unsigned int checksum(unsigned int crc, const unsigned char* data,
                             unsigned int size)
{
    static const unsigned int nibble[16] = {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
    };

    while (size--) {
        crc ^= (unsigned int)*data++ << 8;
        crc = (crc << 4) ^ nibble[crc >> 12];
        crc = (crc << 4) ^ nibble[crc >> 12];
    }
    return crc;
}

static unsigned char swap_dma(unsigned char slot, unsigned int offset,
                              void* data, unsigned int size, unsigned char command)
{
    unsigned char status;
    if (!size) return 1;
    reu_offset[2] = slot;
    reu_position = offset;
    reu_buffer = data;
    reu_length = size;
    status = REU_STATUS;
    krnl_reu_transfer(command);
    status = REU_STATUS;
    return (status & 0x60) == 0x40;
}

static unsigned char write_verified(unsigned char slot, unsigned int offset,
                                    const void* data, unsigned int size)
{
    return swap_dma(slot, offset, (void*)data, size, 0x90) &&
           swap_dma(slot, offset, (void*)data, size, 0x93);
}

static unsigned char verify(unsigned char slot, unsigned int size,
                            const unsigned char* header)
{
    unsigned char readback[HEADER_SIZE];
    unsigned int crc = 0xffffu, offset = HEADER_SIZE, chunk;

    if (!swap_dma(slot, 0, readback, HEADER_SIZE, 0x91) ||
        memcmp(readback, header, HEADER_SIZE)) return 0;
    while (offset < size) {
        chunk = size - offset;
        if (chunk > KRNL_BUFFER_SIZE) chunk = KRNL_BUFFER_SIZE;
        if (!swap_dma(slot, offset, KRNL_REQUEST.data, chunk, 0x91)) return 0;
        crc = checksum(crc, KRNL_REQUEST.data, chunk);
        offset += chunk;
    }
    return (unsigned char)crc == header[7] && (unsigned char)(crc >> 8) == header[8];
}

unsigned char krnl_swap_save(const char* name, const void* context,
                               unsigned int context_size)
{
    unsigned char header[HEADER_SIZE], slot;
    unsigned int address, offset, crc = 0xffffu;

    if (!mounted || !swap_name(name) || context_size > 0xffffu - HEADER_SIZE - BANK1_SIZE ||
        krnl_reu_find(name) || !(slot = krnl_reu_find(""))) return 1;
    memcpy(header, signature, sizeof(signature));
    header[5] = (unsigned char)context_size;
    header[6] = (unsigned char)(context_size >> 8);
    crc = checksum(crc, context, context_size);
    if (!write_verified(slot, HEADER_SIZE, context, context_size)) return 1;

    offset = HEADER_SIZE + context_size;
    for (address = BANK1_START; address < BANK1_END; address += KRNL_BUFFER_SIZE) {
        KRNL_REQUEST.count = KRNL_BUFFER_SIZE;
        krnl_bank1_fetch(address);
        crc = checksum(crc, KRNL_REQUEST.data, KRNL_BUFFER_SIZE);
        if (!write_verified(slot, offset, KRNL_REQUEST.data, KRNL_BUFFER_SIZE)) return 1;
        offset += KRNL_BUFFER_SIZE;
    }
    header[7] = (unsigned char)crc;
    header[8] = (unsigned char)(crc >> 8);
    if (!write_verified(slot, 0, header, HEADER_SIZE)) return 1;

    krnl_reu_publish(slot, name, offset, KRNL_SWAP_TYPE);
    return 0;
}

unsigned char krnl_swap_available(unsigned char slot, unsigned int context_size)
{
    unsigned char header[HEADER_SIZE];

    return reu_dir_size == HEADER_SIZE + context_size + BANK1_SIZE &&
           swap_dma(slot + 1, 0, header, HEADER_SIZE, 0x91) &&
           !memcmp(header, signature, sizeof(signature)) &&
           header[5] == (unsigned char)context_size &&
           header[6] == (unsigned char)(context_size >> 8);
}

unsigned char krnl_swap_restore(const char* name, void* context,
                                  unsigned int context_size)
{
    unsigned char header[HEADER_SIZE], slot;
    unsigned int address, offset;

    if (!mounted || !swap_name(name) || context_size > 0xffffu - HEADER_SIZE - BANK1_SIZE ||
        !(slot = krnl_reu_find(name)) || reu_dir_type != KRNL_SWAP_TYPE ||
        reu_dir_size != HEADER_SIZE + context_size + BANK1_SIZE) return 1;
    if (!swap_dma(slot, 0, header, HEADER_SIZE, 0x91) ||
        memcmp(header, signature, sizeof(signature)) ||
        header[5] != (unsigned char)context_size ||
        header[6] != (unsigned char)(context_size >> 8) ||
        !verify(slot, reu_dir_size, header)) return 1;

    if (!swap_dma(slot, HEADER_SIZE, context, context_size, 0x91)) return 1;
    offset = HEADER_SIZE + context_size;
    for (address = BANK1_START; address < BANK1_END; address += KRNL_BUFFER_SIZE) {
        if (!swap_dma(slot, offset, KRNL_REQUEST.data, KRNL_BUFFER_SIZE, 0x91)) return 1;
        KRNL_REQUEST.count = KRNL_BUFFER_SIZE;
        krnl_bank1_store(address);
        offset += KRNL_BUFFER_SIZE;
    }
    return 0;
}
