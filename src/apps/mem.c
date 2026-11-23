#include <conio.h>

struct segment {
    const char* name;
    unsigned int start;
    unsigned int end;
    unsigned char is_free;
};

static const struct segment bank0[] = {
#include "cmdos-memory.inc"
};

static const struct segment bank1[] = {
    { "RAM comun durante la ejecucion", 0x0000, 0x0FFF, 0 },
    { "Area CMDP (codigo, datos y BSS)", 0x1000, 0x33FF, 0 },
    { "Pila C CMDP (reserva)", 0x3400, 0x37FF, 0 },
    { "Libre para datos (bufer de EDIT)", 0x3800, 0xBFFF, 1 }
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

static void show_bank(const char* title, const struct segment* segments,
                      unsigned char count)
{
    unsigned char index;
    unsigned int assigned = 0;
    unsigned int free = 0;

    cputs(title);
    cputs("Inicio   Fin       KiB     Segmento (fin inclusivo)\r\n");
    for (index = 0; index < count; ++index) {
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
    cputs(" KiB  RAM libre sin asignar: ");
    put_size(free);
    cputs(" KiB\r\n");
}

#include "../kernel/cmdosk-abi.h"

int main(void)
{
    show_bank("\r\nCMD-OS: mapa de memoria del banco 0\r\n",
              bank0, sizeof(bank0) / sizeof(bank0[0]));
    cputs("Color VIC-II: $D800-$D823, 36 celdas de 4 bits (18 bytes).\r\n");
    show_bank("CMD-OS: mapa de memoria del banco 1\r\n",
              bank1, sizeof(bank1) / sizeof(bank1[0]));
    KRNL_REQUEST.operation = KRNL_REU_STATUS;
    ((void (*)(void))0x0E00)();
    if (KRNL_REQUEST.result == KRNL_REU_16MIB)
        cputs("REU: 16 MiB (banco de 8 MiB verificado)\r\n");
    else if (KRNL_REQUEST.result == KRNL_REU_PRESENT)
        cputs("REU: detectada, capacidad sin confirmar\r\n");
    else
        cputs("REU: no detectada\r\n");
    return 0;
}
