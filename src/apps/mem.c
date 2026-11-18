#include <conio.h>

struct segment {
    const char* name;
    unsigned int start;
    unsigned int end;
    unsigned char is_free;
};

static const struct segment segments[] = {
#include "cmdos-memory.inc"
};

static void put_address(unsigned int address)
{
    static const char digits[] = "0123456789ABCDEF";
    char text[6];
    unsigned char count;

    text[0] = '$';
    for (count = 1; count < 5; ++count) {
        text[count] = digits[address >> 12];
        address <<= 4;
    }
    text[5] = '\0';
    cputs(text);
}

static void put_size(unsigned int bytes)
{
    unsigned int hundredths = (bytes >> 10) * 100
                            + ((bytes & 1023) * 25 + 128) / 256;
    unsigned char whole = (unsigned char)(hundredths / 100);
    unsigned char fraction = (unsigned char)(hundredths % 100);
    char text[] = " 0.00";

    if (whole >= 10) {
        text[0] = '0' + whole / 10;
    }
    text[1] = '0' + whole % 10;
    text[3] = '0' + fraction / 10;
    text[4] = '0' + fraction % 10;
    cputs(text);
}

void main(void)
{
    unsigned char index;
    unsigned int assigned = 0;
    unsigned int free = 0;

    cputs("\r\nCMD-OS: mapa de memoria del banco 0\r\n");
    cputs("Inicio   Fin       KiB     Segmento (fin inclusivo)\r\n");
    for (index = 0; index < sizeof(segments) / sizeof(segments[0]); ++index) {
        unsigned int bytes = segments[index].end - segments[index].start + 1;

        if (segments[index].is_free) {
            free += bytes;
        } else {
            assigned += bytes;
        }
        put_address(segments[index].start);
        cputs("    ");
        put_address(segments[index].end);
        cputs("    ");
        put_size(bytes);
        cputs("    ");
        cputs(segments[index].name);
        cputs("\r\n");
    }
    cputs("Total asignado: ");
    put_size(assigned);
    cputs(" KiB\r\n");
    cputs("RAM libre sin asignar: ");
    put_size(free);
    cputs(" KiB\r\n");
}
