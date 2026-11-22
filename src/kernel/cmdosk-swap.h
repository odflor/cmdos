#ifndef CMDOSK_SWAP_H
#define CMDOSK_SWAP_H

#define CMDOSK_SWAP_TYPE 0x80

/* Call with the Bank 1 gate installed and with all file channels closed.
 * The caller owns the capture and restoration of the CPU context.
 * Returns zero on success. Save requires a new .swp name. */
unsigned char cmdosk_swap_save(const char* name, const void* context,
                               unsigned int context_size);
unsigned char cmdosk_swap_restore(const char* name, void* context,
                                  unsigned int context_size);

#endif
