#ifndef CMDOS_FILENAME_H
#define CMDOS_FILENAME_H

#define KRNL_FILENAME_MAX 16

/* Character literals follow the compiler's C128 PETSCII mapping. */
static unsigned char krnl_valid_filename(const char* name)
{
    unsigned char ch, length = 0, dot = 0;

    while ((ch = (unsigned char)*name++) != 0) {
        if (length == KRNL_FILENAME_MAX) return 0;
        if (ch == '.') {
            if (!length || dot || !*name) return 0;
            dot = 1;
        } else if (!((ch >= 'a' && ch <= 'z') ||
                     (ch >= 'A' && ch <= 'Z') ||
                     (ch >= '0' && ch <= '9') || ch == '-' || ch == '_')) {
            return 0;
        }
        ++length;
    }
    return length != 0;
}

#endif
